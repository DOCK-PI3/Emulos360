param([Parameter(Mandatory=$true)][string]$InstallDirectory)
$ErrorActionPreference='Stop'
$target=(Resolve-Path -LiteralPath $InstallDirectory).Path
$updates=Join-Path $target 'updates'
$destination=Join-Path $updates 'apply-update-windows.ps1'
$source=Join-Path $PSScriptRoot '../updates/apply-update-windows.ps1'
foreach($path in @($target,$updates,$destination)) {
    $item=Get-Item -LiteralPath $path -Force
    if($item.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw "No se permiten enlaces: $path" }
}
foreach($relative in @('Emulos360.exe','engine/Emulos360-core.exe')) {
    if(!(Test-Path -LiteralPath (Join-Path $target $relative) -PathType Leaf)) { throw 'Selecciona una instalacion completa de Emulos360.' }
}
if(!(Test-Path -LiteralPath $destination -PathType Leaf)) { throw 'Falta el ayudante instalado.' }
foreach($process in @(Get-Process -Name Emulos360,Emulos360-core -ErrorAction SilentlyContinue)) {
    if($process.Path -and [IO.Path]::GetDirectoryName($process.Path).StartsWith($target,[StringComparison]::OrdinalIgnoreCase)) {
        throw 'Cierra Emulos360 y el juego antes de reparar el actualizador.'
    }
}
$tokens=$null; $parseErrors=$null
[Management.Automation.Language.Parser]::ParseFile($source,[ref]$tokens,[ref]$parseErrors) | Out-Null
if($parseErrors.Count) { throw 'El ayudante nuevo contiene errores de sintaxis.' }
$oldHash=(Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash
$newHash=(Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash
if($oldHash -eq $newHash) { Write-Output 'El ayudante corregido ya esta instalado.'; exit 0 }
$backup=Join-Path $updates 'apply-update-windows.before-single-core.ps1'
if(Test-Path -LiteralPath $backup) { $backup=Join-Path $updates ('apply-update-windows.before-single-core.'+[guid]::NewGuid().ToString('N')+'.ps1') }
Copy-Item -LiteralPath $destination -Destination $backup
$staged=Join-Path $updates ('.apply-update-windows.'+[guid]::NewGuid().ToString('N')+'.ps1')
try {
    Copy-Item -LiteralPath $source -Destination $staged
    if((Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash -ne $oldHash) { throw 'El ayudante instalado ha cambiado durante la reparacion.' }
    [IO.File]::Replace($staged,$destination,$backup)
} finally {
    if(Test-Path -LiteralPath $staged) { Remove-Item -LiteralPath $staged -Force }
}
Write-Output "Ayudante corregido: $destination"
Write-Output "Copia anterior: $backup"
Write-Output 'Abre Emulos360 y repite la actualizacion a 0.5.0. No se modificaron perfiles ni partidas.'
