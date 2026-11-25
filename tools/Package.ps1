param([string]$Build = 'build', [string]$Output = 'dist', [string]$Version = '', [string]$Tag = '', [switch]$Sign)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
Set-Location $root
$match = [regex]::Match((Get-Content CMakeLists.txt -Raw), 'project\(DesktopPet VERSION (\d+\.\d+\.\d+)')
$sourceVersion = $match.Groups[1].Value
if (!$Version) { $Version = $sourceVersion }
if ($Version -cne $sourceVersion) { throw 'Source version mismatch.' }
if ($Tag -and $Tag -cne "v$Version") { throw 'Tag must equal v + project version.' }
if (!(Test-Path "$Build/Release/DesktopPet.exe") -or !(Test-Path "$Build/Release/Updater.exe")) { throw 'Build Release first.' }
$stage = Join-Path $Output 'stage'
if (Test-Path $Output) { Remove-Item $Output -Recurse -Force }
New-Item -ItemType Directory -Force "$stage/data" | Out-Null
Copy-Item "$Build/Release/DesktopPet.exe", "$Build/Release/Updater.exe" $stage
Copy-Item data/dialogue.json "$stage/data/"
Copy-Item README.md, CHANGELOG.md $stage
# Files are explicitly allowlisted; no directory records, deflate, timestamp extras, config or private keys.
Add-Type -AssemblyName System.IO.Compression
$zip = [IO.Path]::GetFullPath((Join-Path $Output 'DesktopPet-windows-x64.zip'))
$stream = [IO.File]::Create($zip)
$archive = [IO.Compression.ZipArchive]::new($stream, [IO.Compression.ZipArchiveMode]::Create)
try {
 foreach ($name in @('DesktopPet.exe','Updater.exe','data/dialogue.json','README.md','CHANGELOG.md')) {
  $entry = $archive.CreateEntry($name, [IO.Compression.CompressionLevel]::NoCompression)
  $entry.LastWriteTime = [DateTimeOffset]::new(2026,1,1,0,0,0,[TimeSpan]::Zero)
  $entry.ExternalAttributes = 0
  $dest = $entry.Open(); $source = [IO.File]::OpenRead((Join-Path $stage $name))
  try { $source.CopyTo($dest) } finally { $source.Dispose(); $dest.Dispose() }
 }
} finally { $archive.Dispose(); $stream.Dispose() }
& "$Build/Release/windows_tests.exe" --package $zip
if ($LASTEXITCODE) { throw 'Native package security checks failed.' }
$size = (Get-Item $zip).Length
if ($size -gt 15MB) { throw 'Release exceeds 15 MiB.' }
$hash = (Get-FileHash -Algorithm SHA256 $zip).Hash.ToLowerInvariant()
[IO.File]::WriteAllText((Join-Path $Output 'SHA256SUMS.txt'), "$hash  DesktopPet-windows-x64.zip`n", [Text.UTF8Encoding]::new($false))
if ($Sign) {
 if (!$env:UPDATE_SIGNING_KEY -or !$env:DESKTOPPET_PUBLIC_KEY) { throw 'Signing key/public key required; refusing unsigned release.' }
 $key = [Security.Cryptography.ECDsa]::Create()
 try {
  $read = 0
  $key.ImportPkcs8PrivateKey([Convert]::FromBase64String($env:UPDATE_SIGNING_KEY), [ref]$read)
  $p = $key.ExportParameters($false)
  $point = [Convert]::ToHexString($p.Q.X) + [Convert]::ToHexString($p.Q.Y)
  if ($point -cne $env:DESKTOPPET_PUBLIC_KEY.ToUpperInvariant()) { throw 'Signing key does not match built-in public key.' }
  # CMakeCache must use the same pinned public key as this signature.
  $cache = Get-Content "$Build/CMakeCache.txt" -Raw
  if ($cache -notmatch ('(?im)^DESKTOPPET_PUBLIC_KEY:STRING=' + [regex]::Escape($env:DESKTOPPET_PUBLIC_KEY) + '\r?$')) { throw 'Build uses a different trusted key.' }
  $manifest = [ordered]@{schema=1;version=$Version;minVersion='1.0.0';arch='windows-x64';asset='DesktopPet-windows-x64.zip';size=$size;sha256=$hash;notes=(Get-Content CHANGELOG.md -Raw)} | ConvertTo-Json -Compress
  $bytes = [Text.UTF8Encoding]::new($false).GetBytes($manifest)
  if ($bytes.Length -gt 65536) { throw 'Signed manifest exceeds client limit.' }
  $sig = $key.SignData($bytes,[Security.Cryptography.HashAlgorithmName]::SHA256,[Security.Cryptography.DSASignatureFormat]::IeeeP1363FixedFieldConcatenation)
  if ($sig.Length -ne 64) { throw 'Unexpected ECDSA signature size.' }
  [IO.File]::WriteAllBytes((Join-Path $Output 'update.json'),$bytes)
  [IO.File]::WriteAllBytes((Join-Path $Output 'update.sig'),$sig)
  & "$Build/Release/windows_tests.exe" --signature $env:DESKTOPPET_PUBLIC_KEY (Join-Path $Output 'update.json') (Join-Path $Output 'update.sig')
  if ($LASTEXITCODE) { throw 'Native signature verification failed.' }
 } finally { $key.Dispose() }
}
Remove-Item $stage -Recurse -Force
Write-Output "Packaged $zip ($size bytes)"
