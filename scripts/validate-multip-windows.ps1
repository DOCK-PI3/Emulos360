param([switch]$CaptureOnly)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$qt = Join-Path $root '.tools/Qt/6.8.3/msvc2022_64'
$log = Join-Path $root '.tools/multip-server'
$clean = [System.Collections.Generic.Dictionary[string,string]]::new([System.StringComparer]::OrdinalIgnoreCase)
foreach ($entry in [Environment]::GetEnvironmentVariables().GetEnumerator()) { $clean[$entry.Key] = [string]$entry.Value }
$clean['Path'] = "$qt/bin;$root/out/Emulos360-MultiP;" + $env:PATH
$clean['QT_PLUGIN_PATH'] = "$qt/plugins"
$clean['QT_QPA_PLATFORM'] = 'offscreen'
$clean['QT_QUICK_BACKEND'] = 'software'
$clean['QT_QPA_FONTDIR'] = 'C:/Windows/Fonts'
$clean['EMULOS_PRIVATE_SERVER_DIR'] = "$root/out/Emulos360-MultiP/server"
$clean['EMULOS_PRIVATE_PANEL_CAPTURE'] = "$log/windows-host-panel.png"
function Run-Check([string]$Executable, [string[]]$Arguments, [string]$Name) {
    $start = [Diagnostics.ProcessStartInfo]::new()
    $start.FileName = $Executable; $start.WorkingDirectory = $root; $start.UseShellExecute = $false
    $start.CreateNoWindow = $true; $start.RedirectStandardOutput = $true; $start.RedirectStandardError = $true
    foreach ($argument in $Arguments) { $start.ArgumentList.Add($argument) }
    $start.Environment.Clear(); foreach ($entry in $clean.GetEnumerator()) { $start.Environment[$entry.Key] = $entry.Value }
    $process = [Diagnostics.Process]::Start($start)
    $stdout = $process.StandardOutput.ReadToEndAsync(); $stderr = $process.StandardError.ReadToEndAsync()
    if (!$process.WaitForExit(90000)) { $process.Kill($true); throw "Tiempo agotado: $Name" }
    $output = $stdout.GetAwaiter().GetResult() + $stderr.GetAwaiter().GetResult()
    [IO.File]::WriteAllText("$log/$Name.txt", $output)
    Write-Output "$Name exit=$($process.ExitCode)"
    Write-Output $output
    if ($process.ExitCode) { throw "Falló $Name" }
}
if (!$CaptureOnly) {
    Run-Check -Executable "$root/out/Emulos360-MultiP/build/Release/ui_tests.exe" -Arguments @('privateNetplayLifecycle','privatePanelRendering','vimmCurrentCatalogParser','-o',"$log/windows-qt-report.txt,txt") -Name 'windows-qt-tests'
}
Run-Check -Executable "$root/out/Emulos360-MultiP/Emulos360.exe" -Arguments @('--data-dir', "$log/preview-data", '--library', 'D:/XBOX360_GAMES', '--page', 'multiplayer', '--capture', "$log/native-panel.png") -Name 'windows-panel-capture'
