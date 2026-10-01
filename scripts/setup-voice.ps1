$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$sources = @(
    @{ Name = 'libdatachannel'; Repository = 'https://github.com/paullouisageneau/libdatachannel.git'; Tag = 'v0.24.5'; Commit = '443f6934d9007eb7076ab7825ba330f355fcbead'; Submodules = $true },
    @{ Name = 'mbedtls'; Repository = 'https://github.com/Mbed-TLS/mbedtls.git'; Tag = 'v3.6.7'; Commit = '068ff080b369adfac81509f9b57b2afabaf82dc5'; Submodules = $true },
    @{ Name = 'opus'; Repository = 'https://github.com/xiph/opus.git'; Tag = 'v1.6.1'; Commit = '22244de5a79bd1d6d623c32e72bf1954b56235be'; Submodules = $false }
)
foreach ($source in $sources) {
    $path = Join-Path $projectRoot ".tools/$($source.Name)"
    if (!(Test-Path $path)) {
        git clone --depth 1 --branch $source.Tag $source.Repository $path
        if ($LASTEXITCODE) { throw "No se pudo descargar $($source.Name)." }
    }
    $current = (git -C $path rev-parse HEAD).Trim()
    if ($LASTEXITCODE -or $current -ne $source.Commit) {
        throw "$($source.Name) debe estar en la revisión fijada $($source.Commit)."
    }
    if ($source.Submodules) {
        git -C $path submodule update --init --recursive --depth 1
        if ($LASTEXITCODE) { throw "No se pudieron preparar los submódulos de $($source.Name)." }
    }
}
