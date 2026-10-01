param([string]$Version = '')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
if (!$Version) {
    $project = Get-Content -LiteralPath (Join-Path $root 'CMakeLists.txt') -Raw
    if ($project -notmatch 'project\(Emulos360 VERSION (\d+\.\d+\.\d+)') {
        throw 'No se pudo leer la versión de CMakeLists.txt.'
    }
    $Version = $Matches[1]
}
if ($Version -notmatch '^\d+\.\d+\.\d+$') { throw 'La versión debe tener formato X.Y.Z.' }
$out = Join-Path $root 'out'
$source = Join-Path $out 'Emulos360-MultiP'
$release = Join-Path $out "release-v$Version"
$archive = Join-Path $release "Emulos360-v$Version-windows-x64.zip"
$sevenZip = 'C:\Program Files\7-Zip\7z.exe'

foreach ($required in @(
    'Emulos360.exe', 'engine/Emulos360-core.exe', 'tools/7z.exe',
    'tools/iso2god.exe', 'server/private-service.cjs', 'server/node.exe',
    'server/mongodb/bin/mongod.exe', 'XEXplugins/MetroDashboard/default.xex',
    'updates/apply-update-windows.ps1'
)) {
    if (!(Test-Path -LiteralPath (Join-Path $source $required))) {
        throw "Falta en la build Windows: $required"
    }
}
if (!(Test-Path -LiteralPath $sevenZip)) { throw 'Falta 7-Zip.' }
foreach ($private in @('server/data', 'server/logs', 'server/.env', 'server/mongodb/data')) {
    if (Test-Path -LiteralPath (Join-Path $source $private)) {
        throw "Revisa datos privados antes de empaquetar: $private"
    }
}

New-Item -ItemType Directory -Force -Path $release | Out-Null
if (Test-Path -LiteralPath $archive) { throw "Ya existe $archive" }
$stage = Join-Path $out ('.release-stage-' + [guid]::NewGuid().ToString('N'))
$stageFull = [IO.Path]::GetFullPath($stage)
$outFull = [IO.Path]::GetFullPath($out).TrimEnd('\')
if (!$stageFull.StartsWith($outFull + '\', [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Carpeta temporal fuera de out.'
}

try {
    $payload = Join-Path $stage 'Emulos360'
    New-Item -ItemType Directory -Force -Path $payload | Out-Null
    Get-ChildItem -LiteralPath $source -Force | Where-Object {
        $_.Name -notin @('build', 'data', 'linux-build', 'linux-engine-build') -and
        $_.Extension -notin @('.log', '.pdb', '.ilk')
    } | ForEach-Object {
        if ($_.Attributes -band [IO.FileAttributes]::ReparsePoint) {
            throw "Enlace inesperado en el paquete: $($_.FullName)"
        }
        Copy-Item -LiteralPath $_.FullName -Destination $payload -Recurse -Force
    }
    Push-Location $stage
    try {
        & $sevenZip a -tzip -mx=5 $archive 'Emulos360' | Out-Null
        if ($LASTEXITCODE) { throw 'Falló la creación del ZIP.' }
    } finally { Pop-Location }
    & $sevenZip t $archive | Out-Null
    if ($LASTEXITCODE) { throw 'El ZIP no pasó la comprobación de integridad.' }
    $hash = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant()
    [IO.File]::WriteAllText("$archive.sha256", "$hash  $([IO.Path]::GetFileName($archive))`n", [Text.UTF8Encoding]::new($false))
    Write-Output "$archive ($([math]::Round((Get-Item $archive).Length / 1MB, 1)) MiB)"
    Write-Output "SHA256 $hash"
} finally {
    if (Test-Path -LiteralPath $stageFull) {
        $item = Get-Item -LiteralPath $stageFull -Force
        if (!$item.PSIsContainer -or ($item.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
            throw "Carpeta temporal insegura: $stageFull"
        }
        Remove-Item -LiteralPath $stageFull -Recurse -Force
    }
}
