param([string]$Build = 'build', [string]$Output = 'dist', [string]$Version = '', [string]$Tag = '', [switch]$Sign, [string]$TrustChain = '', [switch]$EmergencyBackupSigning)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
Set-Location $root
$match = [regex]::Match((Get-Content CMakeLists.txt -Raw), 'project\(DesktopPet VERSION (\d+\.\d+\.\d+)')
$sourceVersion = $match.Groups[1].Value
if (!$Version) { $Version = $sourceVersion }
if ($Version -cne $sourceVersion) { throw 'Source version mismatch.' }
if ($Tag -and $Tag -cne "v$Version") { throw 'Tag must equal v + project version.' }
if (!(Test-Path "$Build/Release/DesktopPet.exe") -or !(Test-Path "$Build/Release/Updater.exe")) { throw 'Build Release first.' }
foreach ($exe in 'DesktopPet','Updater') { if ([Diagnostics.FileVersionInfo]::GetVersionInfo([IO.Path]::GetFullPath("$Build/Release/$exe.exe")).ProductVersion -cne $Version) { throw 'Executable version does not match project version; rebuild Release.' } }
& "$PSScriptRoot/Test-PE.ps1" -Path @("$Build/Release/DesktopPet.exe","$Build/Release/Updater.exe")
$stage = Join-Path $Output 'stage'
if (Test-Path $Output) { Remove-Item $Output -Recurse -Force }
New-Item -ItemType Directory -Force "$stage/data" | Out-Null
Copy-Item "$Build/Release/DesktopPet.exe", "$Build/Release/Updater.exe" $stage
Copy-Item data/dialogue.json "$stage/data/"
Copy-Item README.md, CHANGELOG.md, LICENSE, THIRD_PARTY_NOTICES.md $stage
# Preserve the five-file updater whitelist while carrying all required notices.
$readme = (Get-Content README.md -Raw) + "`n`n## MIT License`n`n" + (Get-Content LICENSE -Raw) + "`n`n" + (Get-Content THIRD_PARTY_NOTICES.md -Raw) + "`n`n" + (Get-Content SECURITY.md -Raw)
[IO.File]::WriteAllText((Join-Path $stage 'README.md'),$readme,[Text.UTF8Encoding]::new($false))
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
# A separate human-download package; it never replaces the updater asset.
$portableName = "DesktopPet_v$Version"
$manualZip = [IO.Path]::GetFullPath((Join-Path $Output "$portableName.zip"))
$manualStream = [IO.File]::Create($manualZip)
$manualArchive = [IO.Compression.ZipArchive]::new($manualStream,[IO.Compression.ZipArchiveMode]::Create)
try {
 foreach ($name in @('DesktopPet.exe','Updater.exe','data/dialogue.json','README.md','CHANGELOG.md','LICENSE','THIRD_PARTY_NOTICES.md')) {
  $entry=$manualArchive.CreateEntry("$portableName/$name",[IO.Compression.CompressionLevel]::NoCompression)
  $entry.LastWriteTime=[DateTimeOffset]::new(2026,1,1,0,0,0,[TimeSpan]::Zero);$entry.ExternalAttributes=0
  $destination=$entry.Open();$source=[IO.File]::OpenRead((Join-Path $stage $name))
  try { $source.CopyTo($destination) } finally { $source.Dispose();$destination.Dispose() }
 }
} finally { $manualArchive.Dispose();$manualStream.Dispose() }
# Verify exact roots and payload identity for BOTH distributions before signing.
function Test-Archive([string]$Path,[string]$Prefix,[string[]]$Names) {
 $file=[IO.File]::OpenRead($Path);$reader=[IO.Compression.ZipArchive]::new($file,[IO.Compression.ZipArchiveMode]::Read)
 try {
  if ($reader.Entries.Count -ne $Names.Count) { throw 'Unexpected archive entry count.' }
  $seen=[Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
  foreach ($entry in $reader.Entries) {
   if (!$seen.Add($entry.FullName) -or !$entry.FullName.StartsWith($Prefix,[StringComparison]::Ordinal)) { throw 'Duplicate or unexpected ZIP root.' }
   $name=$entry.FullName.Substring($Prefix.Length)
   if ($Names -cnotcontains $name) { throw 'Unexpected archive file.' }
   $content=$entry.Open();$hasher=[Security.Cryptography.SHA256]::Create()
   try { $actual=[Convert]::ToHexString($hasher.ComputeHash($content)) } finally { $content.Dispose();$hasher.Dispose() }
   if ($actual -cne (Get-FileHash -Algorithm SHA256 (Join-Path $stage $name)).Hash) { throw 'Archive payload mismatch.' }
  }
 } finally { $reader.Dispose();$file.Dispose() }
}
Test-Archive $zip '' @('DesktopPet.exe','Updater.exe','data/dialogue.json','README.md','CHANGELOG.md')
Test-Archive $manualZip "$portableName/" @('DesktopPet.exe','Updater.exe','data/dialogue.json','README.md','CHANGELOG.md','LICENSE','THIRD_PARTY_NOTICES.md')
& "$Build/Release/windows_tests.exe" --package $zip
if ($LASTEXITCODE) { throw 'Native package security checks failed.' }
# Sign only this version's release notes; old clients also receive concise notes.
$changelog = Get-Content CHANGELOG.md -Raw
$pattern = '(?ms)^##\s+\[?v?' + [regex]::Escape($Version) + '(?![0-9.])[^\r\n]*\r?\n(.*?)(?=^##\s|\z)'
$section = [regex]::Match($changelog, $pattern)
if (!$section.Success) { throw 'Current version changelog section required.' }
$releaseNotes = $section.Groups[1].Value.Trim() -replace '（未發布）|\(未發布\)|未發布',''
if (!$releaseNotes) { throw 'Current version release notes are empty.' }
$size = (Get-Item $zip).Length
if ($size -gt 15MB) { throw 'Release exceeds 15 MiB.' }
$hash = (Get-FileHash -Algorithm SHA256 $zip).Hash.ToLowerInvariant()
$manualHash = (Get-FileHash -Algorithm SHA256 $manualZip).Hash.ToLowerInvariant()
if ((Get-Item $manualZip).Length -gt 15MB) { throw 'Portable release exceeds 15 MiB.' }
[IO.File]::WriteAllText((Join-Path $Output 'SHA256SUMS.txt'), "$hash  DesktopPet-windows-x64.zip`n$manualHash  $portableName.zip`n", [Text.UTF8Encoding]::new($false))
$repository = $env:GITHUB_REPOSITORY
if (!$repository) {
 $buildCache=Get-Content "$Build/CMakeCache.txt" -Raw
 $owner=[regex]::Match($buildCache,'(?m)^DESKTOPPET_OWNER:STRING=([^\r\n]+)').Groups[1].Value
 $repo=[regex]::Match($buildCache,'(?m)^DESKTOPPET_REPOSITORY:STRING=([^\r\n]+)').Groups[1].Value
 if ($owner -and $repo) { $repository="$owner/$repo" }
}
$manualLink=if ($repository) { "[$portableName.zip](https://github.com/$repository/releases/download/v$Version/$portableName.zip)" } else { "$portableName.zip" }
$releaseDescription="## Download / 下載 / ダウンロード`n`nDownload $manualLink for manual installation. Extract the whole $portableName folder and run DesktopPet.exe.`n`n手動下載請選擇 $manualLink，解壓後執行同名資料夾內的 DesktopPet.exe。`n`n手動ダウンロードは $manualLink を選び、同名フォルダー全体を展開して DesktopPet.exe を実行してください。`n`nDesktopPet-windows-x64.zip is reserved for the compatible automatic updater. update.json still points to this flat five-file package. SHA256SUMS.txt covers both ZIP files.`n`n## Changes / 更新內容 / 変更内容`n`n$releaseNotes`n"
[IO.File]::WriteAllText((Join-Path $Output 'RELEASE_NOTES.md'),$releaseDescription,[Text.UTF8Encoding]::new($false))
if ($Sign) {
 if (!$env:UPDATE_SIGNING_KEY -or !$env:DESKTOPPET_PUBLIC_KEY) { throw 'Signing key/public key required; refusing unsigned release.' }
 $key = [Security.Cryptography.ECDsa]::Create()
 try {
  $read = 0
  $key.ImportPkcs8PrivateKey([Convert]::FromBase64String($env:UPDATE_SIGNING_KEY), [ref]$read)
  $p = $key.ExportParameters($false)
  $point = [Convert]::ToHexString($p.Q.X) + [Convert]::ToHexString($p.Q.Y)
  $signingPoint = if ($env:UPDATE_SIGNING_PUBLIC_KEY) { $env:UPDATE_SIGNING_PUBLIC_KEY } else { $env:DESKTOPPET_PUBLIC_KEY }
  if ($point -cne $signingPoint.ToUpperInvariant()) { throw 'Private key does not match declared active signing public key.' }
  if (!$TrustChain -and (Test-Path 'trust/chain.json')) { $TrustChain = 'trust/chain.json' }
  $temporaryChain = ''
  if (!$TrustChain) { $temporaryChain = Join-Path $Output 'empty-trust-chain.json'; [IO.File]::WriteAllText($temporaryChain,'[]'); $TrustChain=$temporaryChain }
  $chain = @(Get-Content $TrustChain -Raw | ConvertFrom-Json)
  if ($EmergencyBackupSigning) { & "$Build/Release/windows_tests.exe" --trust-chain $TrustChain $signingPoint --emergency }
  else { & "$Build/Release/windows_tests.exe" --trust-chain $TrustChain $signingPoint }
  if ($LASTEXITCODE) { throw 'Signing key/policy chain not authorized by embedded trust anchors.' }
  if ($chain.Count) { $policy = $chain[-1].payload | ConvertFrom-Json; if ([version]$Version -lt [version]$policy.minimumVersion) { throw 'Release below trust policy version floor.' } }
  # CMakeCache must use the same pinned public key as this signature.
  $cache = Get-Content "$Build/CMakeCache.txt" -Raw
  if ($cache -notmatch ('(?im)^DESKTOPPET_PUBLIC_KEY:STRING=' + [regex]::Escape($env:DESKTOPPET_PUBLIC_KEY) + '\r?$')) { throw 'Build uses a different trusted key.' }
  $manifestObject = [ordered]@{schema=1;version=$Version;minVersion='1.0.0';arch='windows-x64';asset='DesktopPet-windows-x64.zip';size=$size;sha256=$hash;notes=$releaseNotes}
  if ($chain.Count) { $manifestObject['trust']=$chain }
  $manifest = $manifestObject | ConvertTo-Json -Depth 12 -Compress
  $bytes = [Text.UTF8Encoding]::new($false).GetBytes($manifest)
  if ($bytes.Length -gt 65536) { throw 'Signed manifest exceeds client limit.' }
  $sig = $key.SignData($bytes,[Security.Cryptography.HashAlgorithmName]::SHA256,[Security.Cryptography.DSASignatureFormat]::IeeeP1363FixedFieldConcatenation)
  if ($sig.Length -ne 64) { throw 'Unexpected ECDSA signature size.' }
  [IO.File]::WriteAllBytes((Join-Path $Output 'update.json'),$bytes)
  [IO.File]::WriteAllBytes((Join-Path $Output 'update.sig'),$sig)
  & "$Build/Release/windows_tests.exe" --signature $signingPoint (Join-Path $Output 'update.json') (Join-Path $Output 'update.sig')
  if ($LASTEXITCODE) { throw 'Native signature verification failed.' }
  if ($temporaryChain) { Remove-Item -LiteralPath $temporaryChain -Force }
 } finally { $key.Dispose() }
}
Remove-Item $stage -Recurse -Force
Write-Output "Packaged updater: $zip ($size bytes)"
Write-Output "Packaged manual download: $manualZip"
