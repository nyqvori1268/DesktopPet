# Run in PowerShell 7. Never save the private file inside the repository.
param([Parameter(Mandatory)][string]$PrivateKeyPath)
$ErrorActionPreference = 'Stop'
$path = [IO.Path]::GetFullPath($PrivateKeyPath)
if (Test-Path -LiteralPath $path) { throw 'Refusing to overwrite an existing private key.' }

# NIST P-256 (secp256r1) using its explicit object identifier.
$curve = [Security.Cryptography.ECCurve]::CreateFromValue('1.2.840.10045.3.1.7')
$key = [Security.Cryptography.ECDsa]::Create($curve)
if ($null -eq $key) { throw 'Failed to create ECDSA P-256 key.' }
try {
    $p = $key.ExportParameters($false)
    if ($p.Q.X.Length -ne 32 -or $p.Q.Y.Length -ne 32) {
        throw 'Unexpected P-256 public key size.'
    }
    $point = [Convert]::ToHexString($p.Q.X) + [Convert]::ToHexString($p.Q.Y)
    $private = [Convert]::ToBase64String($key.ExportPkcs8PrivateKey())

    # CreateNew also prevents accidental overwrites if the file appears meanwhile.
    $stream = [IO.File]::Open($path, [IO.FileMode]::CreateNew, [IO.FileAccess]::Write, [IO.FileShare]::None)
    try {
        $bytes = [Text.UTF8Encoding]::new($false).GetBytes($private)
        $stream.Write($bytes, 0, $bytes.Length)
    } finally {
        $stream.Dispose()
    }
    Write-Output "DESKTOPPET_PUBLIC_KEY=$point"
    Write-Output 'Private PKCS#8 Base64 key saved. Set its contents as GitHub Secret UPDATE_SIGNING_KEY.'
} finally {
    $key.Dispose()
}
