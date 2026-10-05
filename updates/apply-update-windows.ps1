param(
    [Parameter(Mandatory)][string]$InstallDir,
    [Parameter(Mandatory)][string]$Archive,
    [Parameter(Mandatory)][int]$AppPid,
    [Parameter(Mandatory)][string]$DataDir,
    [Parameter(Mandatory)][string]$NewsFile,
    [Parameter(Mandatory)][string]$TemporaryDir,
    [switch]$NoLaunch
)
$ErrorActionPreference = 'Stop'
$install = (Resolve-Path -LiteralPath $InstallDir).Path
$data = (Resolve-Path -LiteralPath $DataDir).Path
$archivePath = (Resolve-Path -LiteralPath $Archive).Path
$newsPath = (Resolve-Path -LiteralPath $NewsFile).Path
if (!(Test-Path -LiteralPath (Join-Path $install 'Emulos360.exe') -PathType Leaf)) {
    throw 'La carpeta de instalación no contiene Emulos360.exe.'
}
$deadline = [DateTime]::UtcNow.AddMinutes(2)
while ($AppPid -gt 0 -and (Get-Process -Id $AppPid -ErrorAction SilentlyContinue)) {
    if ([DateTime]::UtcNow -ge $deadline) { throw 'Emulos360 no se cerró en dos minutos.' }
    Start-Sleep -Seconds 1
}
$work = Join-Path (Split-Path $install -Parent) ('.emulos360-update-' + [guid]::NewGuid().ToString('N'))
$extracted = Join-Path $work 'extracted'
$backup = Join-Path $work 'backup'
New-Item -ItemType Directory -Path $extracted,$backup | Out-Null
$installed = [System.Collections.Generic.List[string]]::new()
$saved = [System.Collections.Generic.List[string]]::new()
try {
    Add-Type -AssemblyName System.IO.Compression
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $zip = [IO.Compression.ZipFile]::OpenRead($archivePath)
    try {
        if ($zip.Entries.Count -gt 100000) { throw 'El paquete contiene demasiados archivos.' }
        $total = [long]0
        foreach ($entry in $zip.Entries) {
            $name = $entry.FullName.Replace('\','/')
            if (!$name.StartsWith('Emulos360/', [StringComparison]::Ordinal) -or
                $name.Contains(':') -or $name.Split('/') -contains '..' -or
                $name.Split('/') -contains '.' -or $name.StartsWith('Emulos360/data/')) {
                throw 'El paquete contiene una ruta no permitida.'
            }
            $type = ($entry.ExternalAttributes -shr 16) -band 0xF000
            if ($type -eq 0xA000) { throw 'El paquete contiene un enlace simbólico.' }
            $total += $entry.Length
            if ($total -gt 4GB) { throw 'El paquete descomprimido es demasiado grande.' }
        }
    } finally { $zip.Dispose() }
    Expand-Archive -LiteralPath $archivePath -DestinationPath $extracted
    $source = Join-Path $extracted 'Emulos360'
    foreach ($required in @('Emulos360.exe','engine/Emulos360-core.exe','updates/apply-update-windows.ps1')) {
        if (!(Test-Path -LiteralPath (Join-Path $source $required) -PathType Leaf)) {
            throw "Falta $required en la actualización."
        }
    }
    foreach ($item in Get-ChildItem -LiteralPath $source -Force) {
        if ($item.Name -eq 'data' -or ($item.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
            throw 'El paquete contiene una carpeta personal o un enlace.'
        }
        $destination = Join-Path $install $item.Name
        if (Test-Path -LiteralPath $destination) {
            $existing = Get-Item -LiteralPath $destination -Force
            if ($existing.Attributes -band [IO.FileAttributes]::ReparsePoint) {
                throw 'Se detectó un enlace en la instalación.'
            }
            Move-Item -LiteralPath $destination -Destination (Join-Path $backup $item.Name)
            $saved.Add($item.Name)
        }
        Move-Item -LiteralPath $item.FullName -Destination $destination
        $installed.Add($item.Name)
    }
    Copy-Item -LiteralPath $newsPath -Destination (Join-Path $data 'update-news.json') -Force
    if (!$NoLaunch) {
        $process = Start-Process -FilePath (Join-Path $install 'Emulos360.exe') -ArgumentList @('--data-dir', ('"' + $data + '"')) -WorkingDirectory $install -PassThru
        Start-Sleep -Seconds 3
        if ($process.HasExited) { throw 'La nueva versión se cerró durante el arranque.' }
    }
    Remove-Item -LiteralPath $work -Recurse -Force -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath $TemporaryDir -Recurse -Force -ErrorAction SilentlyContinue
    exit 0
} catch {
    $reason = $_.Exception.Message
    $rollback = ''
    try {
        for ($index = $installed.Count - 1; $index -ge 0; --$index) {
            $destination = Join-Path $install $installed[$index]
            if (Test-Path -LiteralPath $destination) {
                $failed = Join-Path $work ('failed-' + $installed[$index])
                Move-Item -LiteralPath $destination -Destination $failed
            }
        }
        for ($index = $saved.Count - 1; $index -ge 0; --$index) {
            Move-Item -LiteralPath (Join-Path $backup $saved[$index]) -Destination (Join-Path $install $saved[$index])
        }
    } catch { $rollback = "; también falló la restauración: $($_.Exception.Message). Copia: $backup" }
    $message = "No se pudo instalar la actualización: $reason$rollback"
    Remove-Item -LiteralPath (Join-Path $data 'update-news.json') -Force -ErrorAction SilentlyContinue
    [IO.File]::WriteAllText((Join-Path $data 'update-error.txt'), $message)
    [Console]::Error.WriteLine($message)
    if (!$rollback) {
        Remove-Item -LiteralPath $work -Recurse -Force -ErrorAction SilentlyContinue
        if (!$NoLaunch) {
            Start-Process -FilePath (Join-Path $install 'Emulos360.exe') -ArgumentList @('--data-dir', ('"' + $data + '"')) -WorkingDirectory $install
        }
    }
    exit 1
}
