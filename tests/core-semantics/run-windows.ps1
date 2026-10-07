param([string]$BuildDirectory = '')
$ErrorActionPreference = 'Stop'
$coreProject = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
if (!$BuildDirectory) {
    $BuildDirectory = Join-Path $coreProject 'local/core-maintenance-20261006/windows'
}
$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$coreVsPath = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$coreVsPath) { throw 'Faltan las herramientas C++ de Visual Studio.' }
Import-Module "$coreVsPath/Common7/Tools/Microsoft.VisualStudio.DevShell.dll"
Enter-VsDevShell -VsInstallPath $coreVsPath -SkipAutomaticLocation -DevCmdArguments '-arch=x64 -host_arch=x64' | Out-Null
$env:VSLANG = '1033'
New-Item -ItemType Directory -Force -Path $BuildDirectory | Out-Null
foreach ($variant in @('baseline', 'candidate')) {
    $coreValue = Join-Path $coreProject 'engine/xenia/src/xenia/cpu/hir/value.cc'
    if ($variant -eq 'candidate') {
        $coreValue = Join-Path $coreProject 'local/core-maintenance-20261006/candidate/src/xenia/cpu/hir/value.cc'
    }
    $variantBuild = Join-Path $BuildDirectory $variant
    $variantLog = Join-Path $BuildDirectory "$variant-build.log"
    & cmake -S $PSScriptRoot -B $variantBuild -G Ninja -DCMAKE_BUILD_TYPE=Release "-DCORE_VALUE_SOURCE=$coreValue" *> $variantLog
    if ($LASTEXITCODE) { Get-Content -LiteralPath $variantLog -Tail 25; throw "Falló CMake: $variant" }
    & cmake --build $variantBuild *>> $variantLog
    if ($LASTEXITCODE) { Get-Content -LiteralPath $variantLog -Tail 35; throw "Falló la compilación: $variant" }
}
$baselineExecutable = Join-Path $BuildDirectory 'baseline/core_semantics_tests.exe'
& $baselineExecutable --lanes *> (Join-Path $BuildDirectory 'baseline-lanes.log')
$baselineResult = $LASTEXITCODE
Get-Content -LiteralPath (Join-Path $BuildDirectory 'baseline-lanes.log')
Write-Output "baseline_exit=$baselineResult"
if ($baselineResult -ne 1) { throw 'El baseline debe reproducir el error de selección de elementos.' }
& ctest --test-dir (Join-Path $BuildDirectory 'candidate') --output-on-failure *> (Join-Path $BuildDirectory 'candidate-tests.log')
$candidateResult = $LASTEXITCODE
Get-Content -LiteralPath (Join-Path $BuildDirectory 'candidate-tests.log')
if ($candidateResult) { throw 'Fallaron las pruebas del candidato.' }
$candidateExecutable = Join-Path $BuildDirectory 'candidate/core_semantics_tests.exe'
foreach ($mode in @('lanes', 'all-indices')) {
    & $candidateExecutable "--$mode" *> (Join-Path $BuildDirectory "candidate-$mode.log")
    if ($LASTEXITCODE) { throw "Falló el candidato: $mode" }
    Get-Content -LiteralPath (Join-Path $BuildDirectory "candidate-$mode.log")
}
