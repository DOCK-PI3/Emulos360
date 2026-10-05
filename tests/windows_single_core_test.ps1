$ErrorActionPreference = 'Stop'
$project = Split-Path $PSScriptRoot -Parent
$temporaryBase = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\')
$fixture = [IO.Path]::GetFullPath((Join-Path $temporaryBase ('emulos-single-core-test-' + [guid]::NewGuid().ToString('N'))))
if (!$fixture.StartsWith($temporaryBase + '\', [StringComparison]::OrdinalIgnoreCase)) { throw 'Ruta de fixture insegura.' }
function Put-Fixture([string]$path, [string]$text) {
    New-Item -ItemType Directory -Force (Split-Path $path -Parent) | Out-Null
    [IO.File]::WriteAllText($path, $text)
}
try {
    $source = Join-Path $fixture 'source'
    foreach ($name in @('Emulos360.exe','engine/Emulos360-core.exe','tools/7z.exe','tools/iso2god.exe','server/private-service.cjs','server/node.exe','server/mongodb/bin/mongod.exe','XEXplugins/MetroDashboard/default.xex','updates/apply-update-windows.ps1','engine/edge/retired.exe','engine/xblig/Nexia360.exe','data/cores/xblig/saves/keep','LEEME-XBLIG.md')) {
        Put-Fixture (Join-Path $source $name) $name
    }
    $release = Join-Path $fixture 'release'
    & "$project/scripts/package-windows.ps1" -Version 0.4.0 -SourceDirectory $source -ReleaseDirectory $release
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $zip = [IO.Compression.ZipFile]::OpenRead((Join-Path $release 'Emulos360-v0.4.0-windows-x64.zip'))
    try {
        $entries = @($zip.Entries.FullName)
        if ($entries -notcontains 'Emulos360/engine/Emulos360-core.exe') { throw 'Falta el único core.' }
        if ($entries | Where-Object { $_ -match 'engine/(edge|xblig)/|Emulos360/data/|LEEME-XBLIG' }) { throw 'Se incluyó un proveedor retirado o datos.' }
    } finally { $zip.Dispose() }
    if ([IO.File]::ReadAllText((Join-Path $source 'data/cores/xblig/saves/keep')) -ne 'data/cores/xblig/saves/keep') { throw 'Se modificó el guardado origen.' }
    foreach ($case in @('success','missing-core')) {
        $base = Join-Path $fixture $case
        $install = Join-Path $base 'install'
        $data = Join-Path $install 'data'
        $download = Join-Path $base 'download'
        New-Item -ItemType Directory -Force $data,$download | Out-Null
        Put-Fixture "$install/Emulos360.exe" 'old app'
        Put-Fixture "$install/engine/Emulos360-core.exe" 'old engine'
        Put-Fixture "$install/engine/edge/old.exe" 'retired engine'
        Put-Fixture "$data/profile/Account" 'profile'
        Put-Fixture "$data/save" 'main save'
        Put-Fixture "$data/cores/xblig/saves/indie" 'indie save'
        $payload = Join-Path $base 'payload/Emulos360'
        Put-Fixture "$payload/Emulos360.exe" 'new app'
        Put-Fixture "$payload/updates/apply-update-windows.ps1" 'helper'
        if ($case -eq 'success') { Put-Fixture "$payload/engine/Emulos360-core.exe" 'new engine' }
        $archive = Join-Path $download 'release.zip'
        [IO.Compression.ZipFile]::CreateFromDirectory((Split-Path $payload -Parent),$archive)
        $news = Join-Path $download 'news.json'; Put-Fixture $news '{"version":"fixture"}'
        # Windows PowerShell 5.1 wraps expected native stderr as NativeCommandError.
        $savedErrorPreference = $ErrorActionPreference
        try {
            $ErrorActionPreference = 'Continue'
            & powershell.exe -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "$project/updates/apply-update-windows.ps1" -InstallDir $install -Archive $archive -AppPid 0 -DataDir $data -NewsFile $news -TemporaryDir $download -NoLaunch *> "$base/helper.log"
        } finally { $ErrorActionPreference = $savedErrorPreference }
        $code=$LASTEXITCODE
        if ($case -eq 'success') {
            if ($code -ne 0 -or [IO.File]::ReadAllText("$install/engine/Emulos360-core.exe") -ne 'new engine' -or (Test-Path "$install/engine/edge")) { throw 'Falló la actualización sin Edge.' }
        } else {
            if ($code -eq 0 -or [IO.File]::ReadAllText("$install/engine/Emulos360-core.exe") -ne 'old engine' -or !(Test-Path "$install/engine/edge/old.exe")) { throw 'No se conservó la instalación incompleta.' }
        }
        foreach ($item in @(@('profile/Account','profile'),@('save','main save'),@('cores/xblig/saves/indie','indie save'))) {
            if ([IO.File]::ReadAllText((Join-Path $data $item[0])) -ne $item[1]) { throw 'Se modificaron datos durante la actualización.' }
        }
    }
    $global:LASTEXITCODE = 0
    Write-Output 'PASS: Windows packaging excludes retired cores/data; single-core update and invalid-package rollback preserve profiles/saves.'
} finally {
    if (Test-Path -LiteralPath $fixture) {
        $resolved = (Resolve-Path -LiteralPath $fixture).Path
        $item = Get-Item -LiteralPath $resolved -Force
        if (!$resolved.StartsWith($temporaryBase + '\', [StringComparison]::OrdinalIgnoreCase) -or ($item.Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw 'Fixture de limpieza inseguro.' }
        Remove-Item -LiteralPath $resolved -Recurse -Force
    }
}
