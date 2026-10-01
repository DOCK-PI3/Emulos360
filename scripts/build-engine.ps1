param([int]$Jobs = 4)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
Push-Location $projectRoot
try {
    & ./scripts/prepare-engine.ps1
    Copy-Item -LiteralPath (Join-Path $projectRoot 'assets/emulos360.ico') -Destination (Join-Path $projectRoot 'engine/xenia/assets/icon/emulos360.ico')
    $vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
    $vsPath = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (!$vsPath) { throw 'Faltan las herramientas C++ de Visual Studio.' }
    Import-Module "$vsPath/Common7/Tools/Microsoft.VisualStudio.DevShell.dll"
    Enter-VsDevShell -VsInstallPath $vsPath -SkipAutomaticLocation -DevCmdArguments '-arch=x64 -host_arch=x64'
    # Ninja's MSVC dependency scanner expects the English /showIncludes prefix.
    $env:VSLANG = '1033'
    $env:PATH = "$projectRoot/.tools/glslang/bin;$projectRoot/.tools/spirv-build/tools/Release;$env:PATH"
    foreach ($tool in 'glslangValidator','spirv-opt','spirv-dis') {
        if (!(Get-Command $tool -ErrorAction SilentlyContinue)) { throw "Falta $tool. Consulta docs/BUILD.md." }
    }
    if (!(Test-Path engine/xenia/build/CMakeCache.txt)) {
        Push-Location engine/xenia
        try { python xenia-build.py setup; if ($LASTEXITCODE) { throw 'Falló la preparación del motor.' } }
        finally { Pop-Location }
    }
    # CMake's incremental build does not refresh upstream's generated git banner.
    Push-Location engine/xenia
    try {
        $previousVersion = if (Test-Path build/version.h) { Get-Content build/version.h -Raw } else { '' }
        python -c "import runpy; runpy.run_path('xenia-build.py')['generate_version_h']('build')"
        if ($LASTEXITCODE) { throw 'No se pudo actualizar la versión del motor.' }
        if ($previousVersion -ne (Get-Content build/version.h -Raw)) {
            # Also repair caches created with localized, untracked header dependencies.
            $versionSources = @('src/xenia/app/emulator_window.cc','src/xenia/app/updater.cc','src/xenia/app/updater_dialog.cc','src/xenia/base/main_win.cc','src/xenia/ui/windowed_app_main_win.cc','src/xenia/gpu/trace_writer.cc')
            foreach ($source in $versionSources) { cmake -E touch $source }
        }
    } finally { Pop-Location }
    cmake --build engine/xenia/build --config Release --target xenia-app --parallel $Jobs
    if ($LASTEXITCODE) { throw 'Falló la compilación del motor.' }
} finally { Pop-Location }
