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
    # Xenia's setup invokes Git's POSIX submodule helper. DevShell may keep
    # git.exe on PATH while dropping basename, sed, and git-sh-setup.
    $gitRoot = Split-Path (Split-Path (Get-Command git.exe).Source -Parent) -Parent
    $gitUsrBin = Join-Path $gitRoot 'usr/bin'
    $gitMingwBin = Join-Path $gitRoot 'mingw64/bin'
    if (!(Test-Path (Join-Path $gitUsrBin 'basename.exe')) -or
        !(Test-Path (Join-Path $gitUsrBin 'sed.exe'))) {
        throw 'Faltan las utilidades POSIX de Git for Windows.'
    }
    $env:PATH = "$gitUsrBin;$gitMingwBin;$env:PATH"
    # Ninja's MSVC dependency scanner expects the English /showIncludes prefix.
    $env:VSLANG = '1033'
    $env:PATH = "$projectRoot/.tools/glslang/bin;$projectRoot/.tools/spirv-build/tools/Release;$env:PATH"
    function Invoke-CleanProcess([string]$FileName, [string[]]$Arguments, [string]$WorkingDirectory, [string]$LogPrefix = '') {
        # Some desktop terminals provide both Path and PATH. Child tools can
        # pick the stale one, so pass one case-insensitive environment block.
        $variables = [System.Collections.Generic.Dictionary[string,string]]::new(
            [System.StringComparer]::OrdinalIgnoreCase)
        foreach ($entry in [Environment]::GetEnvironmentVariables().GetEnumerator()) {
            if ($entry.Key -ine 'Path') { $variables[$entry.Key] = [string]$entry.Value }
        }
        $variables['Path'] = $env:PATH
        $start = [System.Diagnostics.ProcessStartInfo]::new()
        $start.FileName = (Get-Command $FileName).Source
        $start.WorkingDirectory = $WorkingDirectory
        $start.UseShellExecute = $false
        if ($LogPrefix) {
            $start.RedirectStandardOutput = $true
            $start.RedirectStandardError = $true
        }
        foreach ($argument in $Arguments) { $start.ArgumentList.Add($argument) }
        $start.Environment.Clear()
        foreach ($entry in $variables.GetEnumerator()) { $start.Environment[$entry.Key] = $entry.Value }
        $process = [System.Diagnostics.Process]::Start($start)
        if ($LogPrefix) {
            $stdout = [System.IO.File]::Create("$LogPrefix.stdout.log")
            $stderr = [System.IO.File]::Create("$LogPrefix.stderr.log")
            try {
                $copyOut = $process.StandardOutput.BaseStream.CopyToAsync($stdout)
                $copyErr = $process.StandardError.BaseStream.CopyToAsync($stderr)
                $process.WaitForExit()
                [System.Threading.Tasks.Task]::WaitAll(@($copyOut, $copyErr))
            } finally {
                $stdout.Dispose()
                $stderr.Dispose()
            }
            if ($process.ExitCode) {
                Get-Content "$LogPrefix.stderr.log" -Tail 30
                Get-Content "$LogPrefix.stdout.log" -Tail 30
            }
        } else {
            $process.WaitForExit()
        }
        if ($process.ExitCode) { throw "$FileName terminó con error $($process.ExitCode)." }
    }
    foreach ($tool in 'glslangValidator','spirv-opt','spirv-dis') {
        if (!(Get-Command $tool -ErrorAction SilentlyContinue)) { throw "Falta $tool. Consulta docs/BUILD.md." }
    }
    if (!(Test-Path engine/xenia/build/CMakeCache.txt)) {
        Invoke-CleanProcess 'python.exe' @('xenia-build.py', 'setup') (Join-Path $projectRoot 'engine/xenia')
    }
    # CMake's incremental build does not refresh upstream's generated git banner.
    Push-Location engine/xenia
    try {
        $previousVersion = if (Test-Path build/version.h) { Get-Content build/version.h -Raw } else { '' }
        Invoke-CleanProcess 'python.exe' @('-c', "import runpy; runpy.run_path('xenia-build.py')['generate_version_h']('build')") (Get-Location).Path
        if ($previousVersion -ne (Get-Content build/version.h -Raw)) {
            # Also repair caches created with localized, untracked header dependencies.
            $versionSources = @('src/xenia/app/emulator_window.cc','src/xenia/app/updater.cc','src/xenia/app/updater_dialog.cc','src/xenia/base/main_win.cc','src/xenia/ui/windowed_app_main_win.cc','src/xenia/gpu/trace_writer.cc')
            foreach ($source in $versionSources) { cmake -E touch $source }
        }
    } finally { Pop-Location }
    Invoke-CleanProcess 'cmake.exe' @('--build', 'engine/xenia/build', '--config', 'Release', '--target', 'xenia-app', '--parallel', [string]$Jobs) $projectRoot (Join-Path $projectRoot 'out/build-engine')
} finally { Pop-Location }
