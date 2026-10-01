param([string]$Configuration = 'Release', [switch]$Test, [switch]$Package)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
Push-Location $projectRoot
try {
    & ./scripts/prepare-engine.ps1
    $qtRoot = Join-Path $projectRoot '.tools/Qt/6.8.3/msvc2022_64'
    if (!(Test-Path "$qtRoot/bin/qmake.exe")) { throw 'Qt no está preparado. Ejecuta scripts/setup-qt.ps1.' }
    & ./scripts/setup-voice.ps1
    $buildTests = if ($Test) { 'ON' } else { 'OFF' }
    cmake -S . -B build -G 'Visual Studio 17 2022' -A x64 "-DCMAKE_PREFIX_PATH=$qtRoot" "-DBUILD_TESTING=$buildTests" -DEMULOS_SERVICE_TESTS=OFF -DEMULOS_NETPLAY_TESTS=OFF -DEMULOS_AVATAR_TESTS=OFF
    if ($LASTEXITCODE) { throw 'Falló la configuración de la interfaz.' }
    cmake --build build --config $Configuration --parallel 4
    if ($LASTEXITCODE) { throw 'Falló la compilación de la interfaz.' }
    $originalPath = $env:PATH
    try {
        $env:PATH = "$qtRoot/bin;$originalPath"
        if ($Test) {
            ctest --test-dir build -C $Configuration -R '^(library_tests|ui_tests|voice_party_tests)$' --output-on-failure
            if ($LASTEXITCODE) { throw 'Fallaron las pruebas. Revisa build/*-results.txt.' }
        }
        if ($Package) {
            if ($Configuration -ne 'Release') { throw 'El paquete debe generarse en Release.' }
            New-Item -ItemType Directory -Force out/Emulos360 | Out-Null
            Copy-Item "build/$Configuration/Emulos360.exe" out/Emulos360/Emulos360.exe
            & ./scripts/setup-iso2god.ps1
            $sevenZip = 'C:/Program Files/7-Zip'
            if (!(Test-Path "$sevenZip/7z.exe") -or !(Test-Path "$sevenZip/7z.dll")) {
                throw 'Falta 7-Zip. Instala 7-Zip antes de empaquetar Emulos360.'
            }
            Copy-Item "$sevenZip/7z.exe","$sevenZip/7z.dll","$sevenZip/License.txt" out/Emulos360/tools/ -Force
            $metro = 'XEXplugins/MetroDashboard/dist'
            if (!(Test-Path "$metro/default.xex") -or !(Test-Path "$metro/font.png")) {
                throw 'Falta Dashboard Metro. Ejecuta XEXplugins/MetroDashboard/build.ps1 primero.'
            }
            New-Item -ItemType Directory -Force out/Emulos360/XEXplugins/MetroDashboard | Out-Null
            Copy-Item "$metro/default.xex","$metro/font.png" out/Emulos360/XEXplugins/MetroDashboard/
            New-Item -ItemType Directory -Force out/Emulos360/intro | Out-Null
            Copy-Item assets/intro/aurora.mp4,assets/intro/aurora.png,assets/intro/nova.mp4,assets/intro/nova.png out/Emulos360/intro/ -Force
            $voiceLicenses = @{
                'libdatachannel-LICENSE.txt' = '.tools/libdatachannel/LICENSE'
                'libjuice-LICENSE.txt' = '.tools/libdatachannel/deps/libjuice/LICENSE'
                'usrsctp-LICENSE.md' = '.tools/libdatachannel/deps/usrsctp/LICENSE.md'
                'plog-LICENSE.txt' = '.tools/libdatachannel/deps/plog/LICENSE'
                'mbedtls-LICENSE.txt' = '.tools/mbedtls/LICENSE'
                'opus-LICENSE.txt' = '.tools/opus/COPYING'
            }
            New-Item -ItemType Directory -Force out/Emulos360/licenses | Out-Null
            foreach ($entry in $voiceLicenses.GetEnumerator()) {
                Copy-Item $entry.Value (Join-Path 'out/Emulos360/licenses' $entry.Key) -Force
            }
            $core = 'engine/xenia/build/bin/Windows/Release/Emulos360-core.exe'
            if (Test-Path $core) {
                New-Item -ItemType Directory -Force out/Emulos360/engine | Out-Null
                Copy-Item $core out/Emulos360/engine/Emulos360-core.exe
            } else { Write-Warning 'Falta compilar el motor para crear perfiles y ejecutar juegos.' }
            if (!(Test-Path out/Emulos360/data) -and (Test-Path out/LOS360/data)) {
                Copy-Item out/LOS360/data out/Emulos360/data -Recurse
            }
            & "$qtRoot/bin/windeployqt.exe" --release --no-translations --qmldir app/qml out/Emulos360/Emulos360.exe
            if ($LASTEXITCODE) { throw 'Falló el despliegue de dependencias Qt.' }
        }
    } finally { $env:PATH = $originalPath }
} finally { Pop-Location }
