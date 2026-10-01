$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
Push-Location $projectRoot
try {
    New-Item -ItemType Directory -Force .tools | Out-Null
    $glslangHash = '1B505A8EA63D4921C2B62E62804A25202FE35F0DF3EAA3D7ACAC323BC0A52CEA'
    if (!(Test-Path .tools/glslang/bin/glslangValidator.exe)) {
        Invoke-WebRequest 'https://github.com/KhronosGroup/glslang/releases/download/main-tot/glslang-main-windows-x86_64-release.zip' -OutFile .tools/glslang.zip
        if ((Get-FileHash .tools/glslang.zip -Algorithm SHA256).Hash -ne $glslangHash) {
            throw 'El binario de glslang cambió. Revisar la nueva versión y actualizar el hash antes de continuar.'
        }
        Expand-Archive .tools/glslang.zip .tools/glslang -Force
        Copy-Item .tools/glslang/bin/glslang.exe .tools/glslang/bin/glslangValidator.exe
    }
    $headerRevision = '2a611a970fdbc41ac2e3e328802aed9985352dca'
    $headerRoot = "$projectRoot/.tools/spirv-headers/SPIRV-Headers-$headerRevision".Replace('\','/')
    if (!(Test-Path "$headerRoot/CMakeLists.txt")) {
        Invoke-WebRequest "https://github.com/KhronosGroup/SPIRV-Headers/archive/$headerRevision.zip" -OutFile .tools/spirv-headers.zip
        Expand-Archive .tools/spirv-headers.zip .tools/spirv-headers -Force
    }
    if (!(Test-Path engine/xenia/third_party/SPIRV-Tools/CMakeLists.txt)) {
        throw 'Inicializa primero el submódulo del motor y sus dependencias.'
    }
    cmake -S engine/xenia/third_party/SPIRV-Tools -B .tools/spirv-build -G 'Visual Studio 17 2022' -A x64 "-DSPIRV-Headers_SOURCE_DIR=$headerRoot" -DSPIRV_SKIP_TESTS=ON -DSPIRV_SKIP_EXECUTABLES=OFF
    if ($LASTEXITCODE) { throw 'Falló la configuración de SPIRV-Tools.' }
    cmake --build .tools/spirv-build --config Release --target spirv-opt spirv-dis --parallel 4
    if ($LASTEXITCODE) { throw 'Falló la compilación de SPIRV-Tools.' }
} finally { Pop-Location }
