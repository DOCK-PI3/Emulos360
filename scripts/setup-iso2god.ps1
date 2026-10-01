param([string]$Destination = 'out/Emulos360/tools')
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$source = Join-Path $projectRoot '.tools/iso2god-rs'
$revision = '76207d7d7f89f551c7a5769560c0073e9ffe50e9'
if (!(Test-Path (Join-Path $source '.git'))) {
    New-Item -ItemType Directory -Force $source | Out-Null
    git -C $source init -q
    if ($LASTEXITCODE) { throw 'No se pudo iniciar el repositorio de ISO2GOD.' }
    git -C $source remote add origin https://github.com/iliazeus/iso2god-rs.git
    if ($LASTEXITCODE) { throw 'No se pudo configurar ISO2GOD.' }
}
$current = git -C $source rev-parse HEAD 2>$null
if ($current -ne $revision) {
    git -C $source fetch --depth 1 origin $revision
    if ($LASTEXITCODE) { throw 'No se pudo descargar la versión fijada de ISO2GOD.' }
    git -C $source checkout --detach $revision
    if ($LASTEXITCODE) { throw 'No se pudo seleccionar la versión fijada de ISO2GOD.' }
}
Push-Location $source
try {
    cargo build --release --locked --bin iso2god
    if ($LASTEXITCODE) { throw 'Falló la compilación de ISO2GOD.' }
} finally { Pop-Location }
$target = Join-Path $projectRoot $Destination
New-Item -ItemType Directory -Force $target | Out-Null
Copy-Item (Join-Path $source 'target/release/iso2god.exe') (Join-Path $target 'iso2god.exe') -Force
Copy-Item (Join-Path $source 'LICENSE') (Join-Path $target 'iso2god-LICENSE.txt') -Force
