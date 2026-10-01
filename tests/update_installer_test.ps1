$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$testRoot = Join-Path $root ('out/updater-installer-test-' + [guid]::NewGuid().ToString('N'))
$resolvedRoot = [IO.Path]::GetFullPath($root).TrimEnd('\')
$resolvedTest = [IO.Path]::GetFullPath($testRoot)
if (!$resolvedTest.StartsWith($resolvedRoot + '\', [StringComparison]::OrdinalIgnoreCase)) {
    throw 'La prueba debe estar dentro del proyecto.'
}

function Run-Case([string]$name, [bool]$badData) {
    $case = Join-Path $testRoot $name
    $install = Join-Path $case 'install'
    $data = Join-Path $install 'data'
    $engine = Join-Path $install 'engine'
    $temp = Join-Path $case 'download'
    $payload = Join-Path $case 'payload/Emulos360'
    New-Item -ItemType Directory -Force $data,$engine,$temp,(Join-Path $payload 'engine'),(Join-Path $payload 'updates') | Out-Null
    [IO.File]::WriteAllText((Join-Path $install 'Emulos360.exe'), 'old app')
    [IO.File]::WriteAllText((Join-Path $engine 'Emulos360-core.exe'), 'old engine')
    [IO.File]::WriteAllText((Join-Path $data 'profile.bin'), 'my profile')
    [IO.File]::WriteAllText((Join-Path $payload 'Emulos360.exe'), 'new app')
    [IO.File]::WriteAllText((Join-Path $payload 'engine/Emulos360-core.exe'), 'new engine')
    [IO.File]::WriteAllText((Join-Path $payload 'updates/apply-update-windows.ps1'), 'helper')
    if ($badData) {
        New-Item -ItemType Directory -Force (Join-Path $payload 'data') | Out-Null
        [IO.File]::WriteAllText((Join-Path $payload 'data/profile.bin'), 'injected')
    }
    $archive = Join-Path $temp 'release.zip'
    $news = Join-Path $temp 'news.json'
    [IO.File]::WriteAllText($news, '{"version":"0.3.0","notes":"Novedades"}')
    Compress-Archive -LiteralPath $payload -DestinationPath $archive
    $powershell = Join-Path $env:SystemRoot 'System32/WindowsPowerShell/v1.0/powershell.exe'
    & $powershell -NoProfile -NonInteractive -ExecutionPolicy Bypass -File (Join-Path $root 'updates/apply-update-windows.ps1') $install $archive 0 $data $news $temp -NoLaunch 2>$null
    $exitCode = $LASTEXITCODE
    if (($exitCode -ne 0) -ne $badData) { throw "Resultado inesperado en ${name}: $exitCode" }
    $expectedApp = if ($badData) { 'old app' } else { 'new app' }
    $expectedEngine = if ($badData) { 'old engine' } else { 'new engine' }
    if ([IO.File]::ReadAllText((Join-Path $install 'Emulos360.exe')) -ne $expectedApp -or
        [IO.File]::ReadAllText((Join-Path $engine 'Emulos360-core.exe')) -ne $expectedEngine -or
        [IO.File]::ReadAllText((Join-Path $data 'profile.bin')) -ne 'my profile') {
        throw "La actualización alteró archivos inesperados en $name."
    }
    $message = if ($badData) { 'update-error.txt' } else { 'update-news.json' }
    if (!(Test-Path -LiteralPath (Join-Path $data $message))) { throw "Falta $message en $name." }
}

try {
    Run-Case 'success' $false
    Run-Case 'rejected' $true
    Write-Output 'Windows updater: replacement, profile preservation and malicious data rejection passed'
} finally {
    if (Test-Path -LiteralPath $resolvedTest) {
        $item = Get-Item -LiteralPath $resolvedTest -Force
        if (!$item.PSIsContainer -or ($item.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
            throw "Carpeta de prueba insegura: $resolvedTest"
        }
        Remove-Item -LiteralPath $resolvedTest -Recurse -Force
    }
}
