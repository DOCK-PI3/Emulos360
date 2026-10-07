$ErrorActionPreference = 'Stop'
$coreProject = Split-Path $PSScriptRoot -Parent
$coreBase = Join-Path $coreProject 'engine/xenia'
$coreValueRelative = 'src/xenia/cpu/hir/value.cc'
$coreValue = Join-Path $coreBase $coreValueRelative
$coreCandidateRelative = 'local/core-maintenance-20261006/candidate'
$coreCandidate = Join-Path $coreProject $coreCandidateRelative
$corePatch = Join-Path $coreProject 'patches/xenia-upstream-candidates/cpu-extract-fold.patch'
$coreMetadata = Join-Path $coreProject 'local/core-maintenance-20261006/candidate-source.json'
$coreHead = (& git -C $coreBase rev-parse HEAD).Trim()
if ($LASTEXITCODE -or $coreHead -ne '6dbaa1fefd1e07cc3d5377e763c68fe8073cbe8c') {
    throw 'La base del core cambió. Reauditar el candidato antes de prepararlo.'
}
$sourceHash = (Get-FileHash -LiteralPath $coreValue -Algorithm SHA256).Hash
& git -C $coreBase apply --check $corePatch
if ($LASTEXITCODE) { throw 'El parche ya está integrado o no encaja. Reauditar la fuente actual.' }
if (Test-Path -LiteralPath $coreMetadata) {
    $previous = Get-Content -LiteralPath $coreMetadata -Raw | ConvertFrom-Json
    if ($previous.baselineSha256 -ne $sourceHash) { throw 'La fuente base cambió; no se sobrescribe el candidato anterior.' }
    & git -C $coreProject apply "--directory=$coreCandidateRelative" --reverse --check $corePatch
    if ($LASTEXITCODE) { throw 'El candidato existente tiene cambios; no se sobrescribe.' }
    Write-Output "Candidato ya preparado: $coreCandidate"
    return
}
$candidateValue = Join-Path $coreCandidate $coreValueRelative
if (Test-Path -LiteralPath $candidateValue) {
    throw 'Ya existe una copia sin metadatos. No se sobrescribe; revisar su procedencia.'
}
New-Item -ItemType Directory -Force -Path (Split-Path $candidateValue -Parent) | Out-Null
Copy-Item -LiteralPath $coreValue -Destination $candidateValue
& git -C $coreProject apply "--directory=$coreCandidateRelative" --check $corePatch
if ($LASTEXITCODE) { throw 'El parche no encaja con la copia.' }
& git -C $coreProject apply "--directory=$coreCandidateRelative" $corePatch
if ($LASTEXITCODE) { throw 'No se pudo preparar el candidato.' }
[ordered]@{
    baselineRevision = $coreHead
    baselineSha256 = $sourceHash
    candidateSha256 = (Get-FileHash -LiteralPath $candidateValue -Algorithm SHA256).Hash
    upstream = '5ce7c336df060d043b0a2351889ec8789322d035'
    scope = 'Value::Extract; copia aislada, sin sustituir el core del producto'
} | ConvertTo-Json | Set-Content -LiteralPath $coreMetadata -Encoding utf8
Write-Output "Candidato preparado: $coreCandidate"
