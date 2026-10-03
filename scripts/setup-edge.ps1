param([string]$Destination = '')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$metadata = Get-Content -LiteralPath (Join-Path $root 'engine/edge-release.json') -Raw | ConvertFrom-Json
if (!$Destination) { $Destination = Join-Path $root '.tools/xenia-edge-runtime' }
New-Item -ItemType Directory -Force -Path (Join-Path $root '.tools') | Out-Null
$expected = $metadata.windows.sha256
$archive = Join-Path $root '.tools/xenia-edge-windows.zip'
if (!(Test-Path -LiteralPath $archive) -or (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant() -ne $expected) {
    Invoke-WebRequest -Uri $metadata.windows.url -OutFile $archive
}
if ((Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant() -ne $expected) {
    throw 'El paquete de Canary Edge no coincide con el SHA256 fijado.'
}
New-Item -ItemType Directory -Force -Path $Destination | Out-Null
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip = [IO.Compression.ZipFile]::OpenRead($archive)
try {
    foreach ($entry in $zip.Entries) {
        if ($entry.FullName -notin @('xenia_edge.exe','LICENSE')) { throw 'El paquete de Edge contiene archivos inesperados.' }
    }
} finally { $zip.Dispose() }
Expand-Archive -LiteralPath $archive -DestinationPath $Destination -Force
foreach ($required in @('xenia_edge.exe','LICENSE')) {
    if (!(Test-Path -LiteralPath (Join-Path $Destination $required) -PathType Leaf)) { throw "Falta $required en Canary Edge." }
}
# Match the SDK version and DXIL validator used by this pinned Edge revision.
# Package only redistributable runtime DLLs; the SDK debug layer is not included.
$runtimeDirectory = Join-Path $Destination 'D3D12'
$licensesDirectory = Join-Path $Destination 'licenses'
New-Item -ItemType Directory -Force -Path $runtimeDirectory,$licensesDirectory | Out-Null
foreach ($redist in $metadata.windows.redists) {
    $redistArchive = Join-Path $root ('.tools/' + $redist.archive)
    if (!(Test-Path -LiteralPath $redistArchive) -or
        (Get-FileHash -LiteralPath $redistArchive -Algorithm SHA256).Hash.ToLowerInvariant() -ne $redist.sha256) {
        Invoke-WebRequest -Uri $redist.url -OutFile $redistArchive
    }
    if ((Get-FileHash -LiteralPath $redistArchive -Algorithm SHA256).Hash.ToLowerInvariant() -ne $redist.sha256) {
        throw "SHA256 incorrecto para el runtime de Edge: $($redist.id)"
    }
    $runtimeZip = [IO.Compression.ZipFile]::OpenRead($redistArchive)
    try {
        $dll = $runtimeZip.GetEntry($redist.entry)
        if (!$dll) { throw "Falta $($redist.entry) en el runtime oficial." }
        [IO.Compression.ZipFileExtensions]::ExtractToFile($dll, (Join-Path $runtimeDirectory $redist.file), $true)
        $licenseEntries = @($runtimeZip.Entries | Where-Object { $_.FullName -match '(^|/)LICENSE[^/]*\.txt$' })
        if (!$licenseEntries.Count) { throw "Falta la licencia del runtime $($redist.id)." }
        foreach ($entry in $licenseEntries) {
            $name = $redist.id + '-' + [IO.Path]::GetFileName($entry.FullName)
            [IO.Compression.ZipFileExtensions]::ExtractToFile($entry, (Join-Path $licensesDirectory $name), $true)
        }
    } finally { $runtimeZip.Dispose() }
}
Copy-Item -LiteralPath (Join-Path $root 'engine/edge-release.json') -Destination (Join-Path $Destination 'release.json') -Force
Write-Output "Canary Edge $($metadata.revision) preparado en $Destination"
