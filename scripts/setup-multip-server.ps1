$ErrorActionPreference = 'Stop'
$project = Split-Path $PSScriptRoot -Parent
$cache = Join-Path $project '.tools/multip-server'
$commit = '6abcc4397e0b6e0632307d9418ecae086cc95f48'
$source = Join-Path $cache "source/Xenia-WebServices-$commit"
New-Item -ItemType Directory -Force $cache | Out-Null
if (!(Test-Path "$source/package.json")) {
    Invoke-WebRequest "https://codeload.github.com/AdrianCassar/Xenia-WebServices/zip/$commit" -OutFile "$cache/upstream.zip"
    if ((Get-FileHash "$cache/upstream.zip" -Algorithm SHA256).Hash -ne '185387e1de206dd6d8fff7faeb442d1303ff37c9ae0dd0fbc06c032044b87369') { throw 'Checksum upstream incorrecto' }
    Expand-Archive -LiteralPath "$cache/upstream.zip" -DestinationPath "$cache/source" -Force
}
$node = (Get-Command node.exe).Source
& $node "$project/server/prepare-upstream.cjs" $source
if ($LASTEXITCODE) { throw 'Falló la preparación upstream' }
Push-Location $source
try {
    & npm.cmd ci --ignore-scripts --no-audit --no-fund
    if ($LASTEXITCODE) { throw 'Falló npm ci de Xenia-WebServices' }
    & npm.cmd run build
    if ($LASTEXITCODE) { throw 'Falló la build de Xenia-WebServices' }
    & npm.cmd prune --omit=dev --ignore-scripts --no-audit --no-fund
    if ($LASTEXITCODE) { throw 'Falló la preparación de dependencias de producción' }
} finally { Pop-Location }
Push-Location "$project/server"
try { & npm.cmd ci --ignore-scripts --no-audit --no-fund; if ($LASTEXITCODE) { throw 'Fallaron las dependencias de la pasarela' } }
finally { Pop-Location }
if (!(Test-Path "$cache/mongodb/mongodb-win32-x86_64-windows-8.0.32/bin/mongod.exe")) {
    Invoke-WebRequest 'https://fastdl.mongodb.org/windows/mongodb-windows-x86_64-8.0.32.zip' -OutFile "$cache/mongodb.zip"
    if ((Get-FileHash "$cache/mongodb.zip" -Algorithm SHA256).Hash -ne '5a0675fdec49b544cd5d374ad50a9a594cf5b3278b5ace658aa51cbb4df17177') { throw 'Checksum MongoDB incorrecto' }
    Expand-Archive -LiteralPath "$cache/mongodb.zip" -DestinationPath "$cache/mongodb" -Force
}
Write-Output 'Servidor MultiP preparado. Ejecuta scripts/build-multip.ps1.'
