$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$engine = Join-Path $projectRoot 'engine/xenia'
$base = '6dbaa1fefd1e07cc3d5377e763c68fe8073cbe8c'
if (!(Test-Path (Join-Path $engine 'xenia-build.py'))) { throw 'Falta el submódulo Xenia. Ejecuta git submodule update --init --recursive.' }
$head = (& git -C $engine rev-parse HEAD).Trim()
if ($LASTEXITCODE -or $head -ne $base) { throw "Xenia debe estar en la revisión $base; revisión actual: $head" }
foreach ($name in @('0001-emulos360.patch', '0002-core-maintenance-0.6.0.patch')) {
    $patch = Join-Path $projectRoot "patches/xenia-netplay/$name"
    & git -C $engine apply --reverse --check $patch 2>$null
    if ($LASTEXITCODE -eq 0) { Write-Output "$name ya aplicado al motor."; continue }
    & git -C $engine apply --check $patch
    if ($LASTEXITCODE) { throw "No se puede aplicar $name. Revisa los cambios locales del submódulo." }
    & git -C $engine apply $patch
    if ($LASTEXITCODE) { throw "No se pudo aplicar $name." }
    Write-Output "$name aplicado al motor."
}
