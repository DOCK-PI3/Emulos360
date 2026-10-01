param(
    [Parameter(Mandatory)][string]$ScriptPath,
    [string[]]$ScriptArguments = @(),
    [Parameter(Mandatory)][string]$LogPrefix
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$script = [IO.Path]::GetFullPath((Join-Path $root $ScriptPath))
$rootFull = [IO.Path]::GetFullPath($root).TrimEnd('\')
if (!$script.StartsWith($rootFull + '\', [StringComparison]::OrdinalIgnoreCase) -or
    !(Test-Path -LiteralPath $script)) {
    throw 'El script debe existir dentro del proyecto.'
}
$log = [IO.Path]::GetFullPath((Join-Path $root $LogPrefix))
if (!$log.StartsWith($rootFull + '\', [StringComparison]::OrdinalIgnoreCase)) {
    throw 'El registro debe estar dentro del proyecto.'
}
New-Item -ItemType Directory -Force -Path (Split-Path $log -Parent) | Out-Null

$gitRoot = Split-Path (Split-Path (Get-Command git.exe).Source -Parent) -Parent
$variables = [System.Collections.Generic.Dictionary[string,string]]::new(
    [StringComparer]::OrdinalIgnoreCase)
foreach ($entry in [Environment]::GetEnvironmentVariables().GetEnumerator()) {
    if ($entry.Key -ine 'Path') { $variables[$entry.Key] = [string]$entry.Value }
}
$variables['Path'] = "$gitRoot\usr\bin;$gitRoot\mingw64\bin;$env:PATH"

$start = [Diagnostics.ProcessStartInfo]::new()
$start.FileName = (Get-Command pwsh.exe).Source
$start.WorkingDirectory = $root
$start.UseShellExecute = $false
$start.CreateNoWindow = $true
$start.RedirectStandardOutput = $true
$start.RedirectStandardError = $true
$start.ArgumentList.Add('-NoProfile')
$start.ArgumentList.Add('-File')
$start.ArgumentList.Add($script)
foreach ($argument in $ScriptArguments) { $start.ArgumentList.Add($argument) }
$start.Environment.Clear()
foreach ($entry in $variables.GetEnumerator()) { $start.Environment[$entry.Key] = $entry.Value }

$process = [Diagnostics.Process]::Start($start)
$stdout = [IO.File]::Create("$log.stdout.log")
$stderr = [IO.File]::Create("$log.stderr.log")
try {
    $copyOut = $process.StandardOutput.BaseStream.CopyToAsync($stdout)
    $copyErr = $process.StandardError.BaseStream.CopyToAsync($stderr)
    $process.WaitForExit()
    [Threading.Tasks.Task]::WaitAll(@($copyOut, $copyErr))
} finally {
    $stdout.Dispose()
    $stderr.Dispose()
}
if ($process.ExitCode) {
    Get-Content "$log.stderr.log" -Tail 30
    Get-Content "$log.stdout.log" -Tail 30
    throw "$ScriptPath terminó con error $($process.ExitCode)."
}
Write-Output "$ScriptPath terminó correctamente. Registros: $log.stdout.log y $log.stderr.log"
