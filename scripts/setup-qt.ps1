$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
Push-Location $projectRoot
try {
    if (!(Test-Path .tools/venv/Scripts/python.exe)) {
        python -m venv .tools/venv
        if ($LASTEXITCODE) { throw 'No se pudo crear el entorno de herramientas.' }
    }
    & .tools/venv/Scripts/python.exe -m pip install aqtinstall==3.3.0
    if ($LASTEXITCODE) { throw 'No se pudo instalar aqtinstall.' }
    if (!(Test-Path .tools/Qt/6.8.3/msvc2022_64/bin/qmake.exe)) {
        & .tools/venv/Scripts/python.exe -m aqt install-qt windows desktop 6.8.3 win64_msvc2022_64 --outputdir .tools/Qt --archives qtbase qtdeclarative qtsvg qttools
        if ($LASTEXITCODE) { throw 'No se pudo descargar Qt.' }
    }
    if (!(Test-Path .tools/Qt/6.8.3/msvc2022_64/lib/cmake/Qt6Quick3D)) {
        & .tools/venv/Scripts/python.exe -m aqt install-qt windows desktop 6.8.3 win64_msvc2022_64 --outputdir .tools/Qt --noarchives -m qtquick3d qtshadertools qtquicktimeline
        if ($LASTEXITCODE) { throw 'No se pudieron descargar los módulos nativos de avatares 3D.' }
    }
    if (!(Test-Path .tools/Qt/6.8.3/msvc2022_64/lib/cmake/Qt6Multimedia)) {
        & .tools/venv/Scripts/python.exe -m aqt install-qt windows desktop 6.8.3 win64_msvc2022_64 --outputdir .tools/Qt --noarchives -m qtmultimedia
        if ($LASTEXITCODE) { throw 'No se pudo descargar Qt Multimedia para las intros.' }
    }
} finally { Pop-Location }
