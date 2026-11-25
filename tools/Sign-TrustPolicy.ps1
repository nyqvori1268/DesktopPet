# Offline operation only. This script NEVER generates or replaces a signing key.
param([Parameter(Mandatory)][string]$Payload,[Parameter(Mandatory)][string]$OfflinePrivateKeyPath,[Parameter(Mandatory)][string]$Output)
$ErrorActionPreference='Stop'
if (Test-Path $Output) { throw 'Refusing to overwrite an existing signed policy.' }
$bytes=[IO.File]::ReadAllBytes([IO.Path]::GetFullPath($Payload))
if ($bytes.Length -gt 16384) { throw 'Policy size limit.' }
$text=[Text.UTF8Encoding]::new($false,$true).GetString($bytes)
$policy=$text | ConvertFrom-Json
if ($policy.schema -ne 1 -or $policy.epoch -lt 1 -or $policy.epoch -gt 32 -or $policy.primary -notmatch '^[0-9a-fA-F]{128}$' -or $policy.backup -notmatch '^[0-9a-fA-F]{128}$') { throw 'Invalid trust policy.' }
$key=[Security.Cryptography.ECDsa]::Create()
try {
 $read=0;$key.ImportPkcs8PrivateKey([Convert]::FromBase64String((Get-Content -LiteralPath $OfflinePrivateKeyPath -Raw)),[ref]$read)
 $signature=$key.SignData($bytes,[Security.Cryptography.HashAlgorithmName]::SHA256,[Security.Cryptography.DSASignatureFormat]::IeeeP1363FixedFieldConcatenation)
 if ($signature.Length -ne 64) { throw 'P256 key required.' }
 $envelope=[ordered]@{payload=$text;signature=[Convert]::ToHexString($signature).ToLowerInvariant()} | ConvertTo-Json -Compress
 [IO.File]::WriteAllText([IO.Path]::GetFullPath($Output),$envelope,[Text.UTF8Encoding]::new($false))
} finally { $key.Dispose() }
Write-Host 'Policy signed offline. Append to the public chain; native release checks must validate the existing authority before publishing.'
