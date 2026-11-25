# PowerShell 7. Uses the EXISTING trusted public key; never creates a key or bypasses verification.
param(
 [Parameter(Mandatory)][string]$Target,
 [Parameter(Mandatory)][string]$Zip,
 [Parameter(Mandatory)][string]$Manifest,
 [Parameter(Mandatory)][string]$Signature,
 [Parameter(Mandatory)][string]$PublicKey,
 [string]$BackupPublicKey = '',
 [string]$ConfigPath = ''
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.Text.Json
$Target = [IO.Path]::GetFullPath($Target)
$files = @('DesktopPet.exe','Updater.exe','data/dialogue.json','README.md','CHANGELOG.md')
$allowed = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
foreach ($name in $files) { [void]$allowed.Add($name) }
function Assert-PlainPath([string]$path) {
 for ($p = [IO.Path]::GetFullPath($path); $p; $p = [IO.Path]::GetDirectoryName($p)) {
  if ((Test-Path -LiteralPath $p) -and (([IO.File]::GetAttributes($p) -band [IO.FileAttributes]::ReparsePoint) -ne 0)) { throw 'Reparse paths are rejected.' }
 }
}
function Atomic-Bytes([string]$path, [byte[]]$bytes) {
 [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($path)) | Out-Null
 $temporary = "$path.tmp-$([Guid]::NewGuid().ToString('N'))"
 try {
  $stream = [IO.File]::Open($temporary,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::None)
  try { $stream.Write($bytes,0,$bytes.Length); $stream.Flush($true) } finally { $stream.Dispose() }
  [IO.File]::Move($temporary,$path,$true)
 } finally { if (Test-Path -LiteralPath $temporary) { Remove-Item -LiteralPath $temporary -Force } }
}
function Version-Of([string]$text) { if ($text -notmatch '^(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)$') { throw 'Stable SemVer required.' }; return [version]$text }
function Verify-Key([string]$pointHex,[byte[]]$bytes,[byte[]]$sig) {
 if ($pointHex -notmatch '^[a-fA-F0-9]{128}$' -or $sig.Length -ne 64) { return $false }
 $point=[Security.Cryptography.ECPoint]::new();$point.X=[Convert]::FromHexString($pointHex.Substring(0,64));$point.Y=[Convert]::FromHexString($pointHex.Substring(64,64))
 $p=[Security.Cryptography.ECParameters]::new();$p.Curve=[Security.Cryptography.ECCurve]::NamedCurves.nistP256;$p.Q=$point
 $key=[Security.Cryptography.ECDsa]::Create($p)
 try { return $key.VerifyData($bytes,$sig,[Security.Cryptography.HashAlgorithmName]::SHA256,[Security.Cryptography.DSASignatureFormat]::IeeeP1363FixedFieldConcatenation) } finally { $key.Dispose() }
}
function Run-TrustCommand([string]$exe,[string[]]$arguments) {
 $start=[Diagnostics.ProcessStartInfo]::new($exe);$start.UseShellExecute=$false
 foreach ($argument in $arguments) { $start.ArgumentList.Add($argument) }
 $process=[Diagnostics.Process]::Start($start)
 try { if (!$process.WaitForExit(20000) -or $process.ExitCode -ne 0) { throw 'Installed client rejected trust state, policy or update validation.' } } finally { $process.Dispose() }
}
Assert-PlainPath $Target
if ($PublicKey -notmatch '^[0-9a-fA-F]{128}$') { throw 'A previously trusted P256 X||Y public key is required.' }
$manifestInfo = Get-Item -LiteralPath $Manifest
if ($manifestInfo.Length -gt 65536 -or (Get-Item -LiteralPath $Signature).Length -ne 64 -or (Get-Item -LiteralPath $Zip).Length -gt 15MB) { throw 'Update file limits exceeded.' }
$message = [IO.File]::ReadAllBytes([IO.Path]::GetFullPath($Manifest))
$sig = [IO.File]::ReadAllBytes([IO.Path]::GetFullPath($Signature))
$jsonText = [Text.UTF8Encoding]::new($false,$true).GetString($message)
$document = [Text.Json.JsonDocument]::Parse($jsonText)
try { $keys=[Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal); foreach ($property in $document.RootElement.EnumerateObject()) { if (!$keys.Add($property.Name)) { throw 'Duplicate manifest key.' } } } finally { $document.Dispose() }
$info = $jsonText | ConvertFrom-Json
if ($info.schema -ne 1 -or $info.arch -cne 'windows-x64' -or $info.asset -cne 'DesktopPet-windows-x64.zip') { throw 'Incompatible update manifest.' }
$installed = Version-Of ([Diagnostics.FileVersionInfo]::GetVersionInfo((Join-Path $Target 'DesktopPet.exe')).ProductVersion)
$latest = Version-Of $info.version; $minimum = Version-Of $info.minVersion
$primary=$PublicKey.ToLowerInvariant();$backup=$BackupPublicKey.ToLowerInvariant();$epoch=0;$floor=[version]'0.0.0'
if ($backup -and ($backup -notmatch '^[a-f0-9]{128}$' -or $backup -ceq $primary)) { throw 'Invalid distinct backup trust anchor.' }
$revoked=[Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
$chain=@();if ($null -ne $info.trust) { $chain=@($info.trust) }; if ($chain.Count -gt 32) { throw 'Trust chain limit.' }
foreach ($envelope in $chain) {
 if ($envelope.signature -notmatch '^[a-fA-F0-9]{128}$' -or [Text.Encoding]::UTF8.GetByteCount($envelope.payload) -gt 16384) { throw 'Invalid trust envelope.' }
 $authority=if ($backup) { $backup } else { $primary }
 if (!(Verify-Key $authority ([Text.Encoding]::UTF8.GetBytes($envelope.payload)) ([Convert]::FromHexString($envelope.signature)))) { throw 'Trust policy not authorized by an existing authority.' }
 $policy=$envelope.payload | ConvertFrom-Json
 if ($policy.schema -ne 1 -or $policy.epoch -ne $epoch+1 -or $policy.primary -notmatch '^[a-fA-F0-9]{128}$' -or $policy.backup -notmatch '^[a-fA-F0-9]{128}$') { throw 'Replayed or invalid trust policy.' }
 $nextPrimary=$policy.primary.ToLowerInvariant();$nextBackup=$policy.backup.ToLowerInvariant();$nextFloor=Version-Of $policy.minimumVersion
 if ($nextPrimary -ceq $nextBackup -or $nextFloor -lt $floor) { throw 'Invalid trust rotation or floor downgrade.' }
 foreach ($old in @($primary,$backup)) { if ($old -and $old -cne $nextPrimary -and $old -cne $nextBackup) { [void]$revoked.Add($old) } }
 foreach ($revocation in $policy.revoked) { if ($revocation -notmatch '^[a-fA-F0-9]{128}$') { throw 'Invalid revocation key.' };[void]$revoked.Add($revocation.ToLowerInvariant()) }
 if ($revoked.Contains($nextPrimary) -or $revoked.Contains($nextBackup)) { throw 'Revoked keys cannot be restored.' }
 $primary=$nextPrimary;$backup=$nextBackup;$floor=$nextFloor;$epoch++
}
if ($latest -lt $floor -or (!(Verify-Key $primary $message $sig) -and !($backup -and (Verify-Key $backup $message $sig)))) { throw 'Invalid manifest signature/version trust floor.' }
$trustCache=Join-Path $env:LOCALAPPDATA 'DesktopPet/update-trust.bin'
if ($installed -ge [version]'1.1.1') { Run-TrustCommand (Join-Path $Target 'DesktopPet.exe') @('--verify-update',[IO.Path]::GetFullPath($Manifest),[IO.Path]::GetFullPath($Signature),[IO.Path]::GetFullPath($Zip)) }
elseif (Test-Path $trustCache) { throw 'Legacy client cannot safely interpret an existing revocation cache. Use a current trusted client/tool; do not delete the cache.' }

if ($latest -le $installed -or $installed -lt $minimum) { throw 'Downgrade/same version/incompatible update rejected.' }
# Read the ZIP once: verify and extract these same bytes, avoiding a file swap race.
$package = [IO.File]::ReadAllBytes([IO.Path]::GetFullPath($Zip))
$hash = [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($package)).ToLowerInvariant()
if ($package.Length -ne $info.size -or $package.Length -gt 15MB -or $hash -cne $info.sha256) { throw 'SHA-256/size verification failed.' }
$journal = Join-Path $Target '.desktoppet-transaction.json'; $backup = Join-Path $Target '.desktoppet-backup'
if ((Test-Path $journal) -or (Test-Path $backup)) { throw 'Recover the existing transaction before manual migration.' }
foreach ($name in $files) { Assert-PlainPath (Join-Path $Target $name) }
$mutex = [Threading.Mutex]::new($false,'Local\DesktopPet.UpdateTransaction'); $owned = $false
$staging = Join-Path ([IO.Path]::GetTempPath()) ('DesktopPet-verified-'+[Guid]::NewGuid().ToString('N'))
$child = $null; $ready = $null; $changed = $false; $existed = @{}; $snapshot = $null; $journalCreated = $false
try {
 $owned = $mutex.WaitOne(0)
 if (!$owned) { throw 'Another updater is running.' }
 if ((Test-Path $journal) -or (Test-Path $backup)) { throw 'Existing transaction must be recovered first.' }
 [IO.Directory]::CreateDirectory($staging) | Out-Null
 $memory = [IO.MemoryStream]::new($package,$false)
 $archive = [IO.Compression.ZipArchive]::new($memory,[IO.Compression.ZipArchiveMode]::Read)
 try {
  if ($archive.Entries.Count -ne 5) { throw 'Only the five release files are allowed.' }
  $seen = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal); $total = 0L
  foreach ($entry in $archive.Entries) {
   if (!$allowed.Contains($entry.FullName) -or !$seen.Add($entry.FullName) -or (($entry.ExternalAttributes -shr 16 -band 0xf000) -eq 0xa000) -or ($entry.ExternalAttributes -band 0x400) -or $entry.Length -gt 15MB) { throw 'Unsafe or unexpected ZIP entry.' }
   $destination = Join-Path $staging $entry.FullName; [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($destination)) | Out-Null
   $input = $entry.Open(); $output = [IO.File]::Open($destination,[IO.FileMode]::CreateNew)
   try {
    $buffer = [byte[]]::new(16384); $written = 0L
    while (($n = $input.Read($buffer,0,$buffer.Length)) -gt 0) { $written += $n; $total += $n; if ($written -gt $entry.Length -or $total -gt 15MB) { throw 'Expanded ZIP limit exceeded.' }; $output.Write($buffer,0,$n) }
    if ($written -ne $entry.Length) { throw 'Truncated ZIP entry.' }; $output.Flush($true)
   } finally { $input.Dispose(); $output.Dispose() }
  }
 } finally { $archive.Dispose(); $memory.Dispose() }
 foreach ($exe in 'DesktopPet.exe','Updater.exe') { if ((Version-Of ([Diagnostics.FileVersionInfo]::GetVersionInfo((Join-Path $staging $exe)).ProductVersion)) -ne $latest) { throw 'Signed payload version mismatch.' } }
 if (!$ConfigPath) { $ConfigPath = Join-Path $Target 'config.json'; if (!(Test-Path $ConfigPath)) { $ConfigPath = Join-Path $env:LOCALAPPDATA 'DesktopPet/config.json' } }
 $ConfigPath = [IO.Path]::GetFullPath($ConfigPath); Assert-PlainPath $ConfigPath
 if ([IO.Path]::GetFileName($ConfigPath) -cne 'config.json') { throw 'ConfigPath must identify config.json.' }
 if ((Test-Path $ConfigPath) -and (Get-Item $ConfigPath).Length -gt 1MB) { throw 'Configuration size limit.' }
 if (Test-Path $ConfigPath) { $snapshot = [IO.File]::ReadAllBytes($ConfigPath) }
 Write-Host "Signature, SHA-256 and file allowlist verified. $installed -> $latest"
 [void](Read-Host '請保持設定不變，從系統匣選「結束程式」，完成後按 Enter（不會強制終止任何程序）')
 $deadline = [DateTime]::UtcNow.AddSeconds(120)
 do {
  $running = $false
  foreach ($process in [Diagnostics.Process]::GetProcessesByName('DesktopPet')) {
   try { if (!$process.HasExited -and [string]::Equals([IO.Path]::GetFullPath($process.MainModule.FileName),(Join-Path $Target 'DesktopPet.exe'),[StringComparison]::OrdinalIgnoreCase)) { $running = $true } }
   catch { if (!$process.HasExited) { throw 'Unable to verify a DesktopPet process; close it manually and retry.' } }
   finally { $process.Dispose() }
  }
  if ($running) { if ([DateTime]::UtcNow -ge $deadline) { throw 'Application exit timed out; files have not been replaced.' }; Start-Sleep -Milliseconds 250 }
 } while ($running)
 [IO.Directory]::CreateDirectory($backup) | Out-Null
 foreach ($name in $files) { $existed[$name] = Test-Path -LiteralPath (Join-Path $Target $name) }
 $transaction = @{phase='backing';existed=$existed}
 Atomic-Bytes $journal ([Text.UTF8Encoding]::new($false).GetBytes(($transaction | ConvertTo-Json -Depth 4 -Compress))); $journalCreated=$true
 foreach ($name in $files) { if ($existed[$name]) { Atomic-Bytes (Join-Path $backup $name) ([IO.File]::ReadAllBytes((Join-Path $Target $name))) } }
 $transaction.phase = 'prepared'; Atomic-Bytes $journal ([Text.UTF8Encoding]::new($false).GetBytes(($transaction | ConvertTo-Json -Depth 4 -Compress))); $changed = $true
 foreach ($name in $files) { Atomic-Bytes (Join-Path $Target $name) ([IO.File]::ReadAllBytes((Join-Path $staging $name))) }
 if ($null -ne $snapshot) { Atomic-Bytes $ConfigPath $snapshot }
 $transaction.phase = 'installed'; Atomic-Bytes $journal ([Text.UTF8Encoding]::new($false).GetBytes(($transaction | ConvertTo-Json -Depth 4 -Compress)))
 $eventName = 'Local\DesktopPetReady-'+[Guid]::NewGuid().ToString('N')
 $ready = [Threading.EventWaitHandle]::new($false,[Threading.EventResetMode]::ManualReset,$eventName)
 $start = [Diagnostics.ProcessStartInfo]::new((Join-Path $Target 'DesktopPet.exe')); $start.WorkingDirectory=$Target; $start.UseShellExecute=$false; $start.ArgumentList.Add('--updated'); $start.ArgumentList.Add($eventName)
 $child = [Diagnostics.Process]::Start($start)
 if (!$ready.WaitOne(20000) -or $child.WaitForExit(3000)) { throw 'New application failed its readiness/health check.' }
 Run-TrustCommand (Join-Path $Target 'DesktopPet.exe') @('--adopt-update-trust',[IO.Path]::GetFullPath($Manifest),[IO.Path]::GetFullPath($Signature))
 Remove-Item -LiteralPath $journal -Force; Remove-Item -LiteralPath $backup -Recurse -Force; $changed=$false
 Write-Host 'Manual migration completed; configuration and timer snapshot restored.'
} catch {
 $failure = $_
 if ($null -ne $child -and !$child.HasExited) { $child.Kill(); $child.WaitForExit() } # Only the process created above, never a PID lookup.
 if ($changed) {
  foreach ($name in $files) { $to=Join-Path $Target $name; if ($existed[$name]) { Atomic-Bytes $to ([IO.File]::ReadAllBytes((Join-Path $backup $name))) } elseif (Test-Path $to) { Remove-Item -LiteralPath $to -Force } }
  if ($null -ne $snapshot) { Atomic-Bytes $ConfigPath $snapshot }
  Remove-Item -LiteralPath $journal -Force; Remove-Item -LiteralPath $backup -Recurse -Force
 } elseif ($journalCreated -and (Test-Path $journal)) { Remove-Item -LiteralPath $journal -Force; if (Test-Path $backup) { Remove-Item -LiteralPath $backup -Recurse -Force } }
 throw $failure
} finally {
 if ($null -ne $ready) { $ready.Dispose() }; if ($null -ne $child) { $child.Dispose() }
 if (Test-Path $staging) { Remove-Item -LiteralPath $staging -Recurse -Force }
 if ($owned) { $mutex.ReleaseMutex() }; $mutex.Dispose()
}
