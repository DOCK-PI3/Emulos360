$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$engine = Join-Path $projectRoot 'engine/xenia'
$patch = Join-Path $projectRoot 'patches/xenia-netplay/0001-emulos360.patch'
$base = '6dbaa1fefd1e07cc3d5377e763c68fe8073cbe8c'

if (!(Test-Path (Join-Path $engine 'xenia-build.py'))) {
    throw 'Falta el submódulo Xenia. Ejecuta git submodule update --init --recursive.'
}
$head = (& git -C $engine rev-parse HEAD).Trim()
if ($LASTEXITCODE -or $head -ne $base) {
    throw "Xenia debe estar en la revisión $base; revisión actual: $head"
}

& git -C $engine apply --reverse --check $patch 2>$null
if ($LASTEXITCODE -eq 0) {
    Write-Output 'Parche Emulos360 ya aplicado al motor.'
    return
}
& git -C $engine apply --check $patch
if ($LASTEXITCODE) { throw 'El parche no se puede aplicar. Revisa los cambios locales del submódulo.' }
& git -C $engine apply $patch
if ($LASTEXITCODE) { throw 'No se pudo aplicar el parche al motor.' }
Write-Output 'Parche Emulos360 aplicado al motor.'
