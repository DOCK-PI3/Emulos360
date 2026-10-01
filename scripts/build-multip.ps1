param([switch]$SkipBuild)
$ErrorActionPreference = 'Stop'
$project = Split-Path $PSScriptRoot -Parent
$package = Join-Path $project 'out/Emulos360-MultiP'
$build = Join-Path $project 'out/Emulos360-MultiP/build'
$qt = Join-Path $project '.tools/Qt/6.8.3/msvc2022_64'
$upstream = Join-Path $project '.tools/multip-server/source/Xenia-WebServices-6abcc4397e0b6e0632307d9418ecae086cc95f48'
if (!(Test-Path "$upstream/dist/main.js")) { throw 'Prepara Xenia-WebServices antes de empaquetar MultiP.' }
Push-Location $project
try {
    # Collapse Path/PATH inherited from desktop terminals before invoking MSBuild.
    $clean = [System.Collections.Generic.Dictionary[string,string]]::new([System.StringComparer]::OrdinalIgnoreCase)
    foreach ($entry in [Environment]::GetEnvironmentVariables().GetEnumerator()) { $clean[$entry.Key] = [string]$entry.Value }
    function Run-CMake([string[]]$Arguments) {
        $start = [System.Diagnostics.ProcessStartInfo]::new()
        $start.FileName = 'cmake'; $start.WorkingDirectory = $project; $start.UseShellExecute = $false
        $start.RedirectStandardOutput = $true; $start.RedirectStandardError = $true
        foreach ($argument in $Arguments) { $start.ArgumentList.Add($argument) }
        $start.Environment.Clear()
        foreach ($entry in $clean.GetEnumerator()) { $start.Environment[$entry.Key] = $entry.Value }
        $process = [System.Diagnostics.Process]::Start($start)
        $stdout = $process.StandardOutput.ReadToEndAsync(); $stderr = $process.StandardError.ReadToEndAsync()
        $process.WaitForExit()
        $output = $stdout.GetAwaiter().GetResult() + $stderr.GetAwaiter().GetResult()
        [System.IO.File]::AppendAllText((Join-Path $project '.tools/multip-server/windows-native.log'), $output)
        if ($process.ExitCode) { $output -split "`n" | Select-String 'error|fatal|LNK' | Select-Object -Last 12 | ForEach-Object { Write-Output $_.Line } }
        if ($process.ExitCode) { throw "CMake terminó con error $($process.ExitCode)" }
    }
    if (!$SkipBuild) {
        Run-CMake -Arguments @('-S', '.', '-B', $build, '-G', 'Visual Studio 17 2022', '-A', 'x64', "-DCMAKE_PREFIX_PATH=$qt", '-DBUILD_TESTING=ON')
        Run-CMake -Arguments @('--build', $build, '--config', 'Release', '--target', 'los360', 'ui_tests', '--parallel', '4')
    }
    New-Item -ItemType Directory -Force $package,"$package/server","$package/licenses" | Out-Null
    $frontend = if ($SkipBuild) {
        Join-Path $project 'out/Emulos360/Emulos360.exe'
    } else {
        "$build/Release/Emulos360.exe"
    }
    if (!(Test-Path -LiteralPath $frontend)) {
        throw 'Falta la interfaz compilada. Ejecuta scripts/build-ui.ps1 -Package primero.'
    }
    Copy-Item $frontend "$package/Emulos360.exe" -Force
    # Only runtime assets are read from the stable package. Profiles, saves and settings stay independent.
    foreach ($folder in @('engine','intro','tools','XEXplugins','licenses','updates')) {
        $source = Join-Path $project "out/Emulos360/$folder"
        if (Test-Path $source) { Copy-Item $source $package -Recurse -Force }
    }
    Copy-Item "$project/server/private-service.cjs","$project/server/panel.html","$project/server/panel.js","$project/server/package.json","$project/server/package-lock.json" "$package/server/" -Force
    Copy-Item "$project/server/node_modules" "$package/server/" -Recurse -Force
    $node = (Get-Command node.exe).Source
    Copy-Item $node "$package/server/node.exe" -Force
    $nodeLicense = Join-Path $project '.tools/multip-server/Node-LICENSE.txt'
    if (Test-Path $nodeLicense) { Copy-Item $nodeLicense "$package/licenses/Node-LICENSE.txt" -Force }
    $mongo = (Get-ChildItem "$project/.tools/multip-server/mongodb" -Directory | Select-Object -First 1).FullName
    New-Item -ItemType Directory -Force "$package/server/mongodb/bin" | Out-Null
    Copy-Item "$mongo/bin/mongod.exe" "$package/server/mongodb/bin/" -Force
    Get-ChildItem $mongo -File | ForEach-Object { Copy-Item $_.FullName "$package/licenses/MongoDB-$($_.Name)" -Force }
    New-Item -ItemType Directory -Force "$package/server/upstream" | Out-Null
    foreach ($folder in @('dist','node_modules')) { Copy-Item "$upstream/$folder" "$package/server/upstream/" -Recurse -Force }
    New-Item -ItemType Directory -Force "$package/server/upstream/src" | Out-Null
    Copy-Item "$upstream/src/titles" "$package/server/upstream/src/" -Recurse -Force
    Copy-Item "$upstream/settings.json","$upstream/package.json","$upstream/package-lock.json" "$package/server/upstream/" -Force
    Copy-Item "$upstream/LICENSE" "$package/licenses/Xenia-WebServices-LICENSE.txt" -Force
    Copy-Item "$project/docs/PRIVATE_NETPLAY.md" "$package/LEEME-MultiP.md" -Force
    $savedPath = $env:PATH
    try {
        $env:PATH = "$qt/bin;$savedPath"
        & "$qt/bin/windeployqt.exe" --release --no-translations --qmldir "$project/app/qml" "$package/Emulos360.exe"
        if ($LASTEXITCODE) { throw 'Falló el despliegue Qt MultiP' }
    } finally { $env:PATH = $savedPath }
    Write-Output "MultiP preparada en $package"
} finally { Pop-Location }
