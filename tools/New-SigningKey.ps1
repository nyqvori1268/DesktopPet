# Run in PowerShell 7. Never save the private file inside the repository.
param([Parameter(Mandatory)][string]$PrivateKeyPath)
$ErrorActionPreference = 'Stop'
$key = [Security.Cryptography.ECDsa]::Create([Security.Cryptography.ECCurve]::NamedCurves.nistP256)
try {
 if (Test-Path $PrivateKeyPath) { throw 'Refusing to overwrite an existing private key.' }
 $private = [Convert]::ToBase64String($key.ExportPkcs8PrivateKey())
 [IO.File]::WriteAllText([IO.Path]::GetFullPath($PrivateKeyPath), $private, [Text.UTF8Encoding]::new($false))
 $p = $key.ExportParameters($false)
 $point = [Convert]::ToHexString($p.Q.X) + [Convert]::ToHexString($p.Q.Y)
 Write-Output "DESKTOPPET_PUBLIC_KEY=$point"
 Write-Output 'Private PKCS#8 Base64 key saved. Set its contents as GitHub Secret UPDATE_SIGNING_KEY.'
} finally { $key.Dispose() }
