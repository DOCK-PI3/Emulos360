param()
$ErrorActionPreference = 'Stop'
$sdkRoot = $env:XEDK
if (!$sdkRoot) { throw 'Falta XEDK (XDK local).' }
$output = Join-Path $PSScriptRoot 'build'
$payload = Join-Path $PSScriptRoot 'dist'
New-Item -ItemType Directory -Force $output,$payload | Out-Null
python (Join-Path $PSScriptRoot 'tools/font.py') $payload
if ($LASTEXITCODE) { throw 'No se pudo generar la fuente.' }
foreach ($shader in @(@('vs','vs_3_0'),@('solid','ps_3_0'),@('textured','ps_3_0'))) {
    & "$sdkRoot/bin/win32/fxc.exe" /nologo /T $shader[1] /E $shader[0] /Fh "$output/$($shader[0]).h" /Vn "g_$($shader[0])" "$PSScriptRoot/shaders/ui.hlsl"
    if ($LASTEXITCODE) { throw 'Falló un shader.' }
}
& "$sdkRoot/bin/win32/cl.exe" /nologo /c /O2 /MT /W4 /WX /GR- /GS- /D_XBOX /DXBOX /DNDEBUG /D_CRT_SECURE_NO_WARNINGS /I "$sdkRoot/include/xbox" /I $output /I $PSScriptRoot /Fo"$output/dashboard.obj" "$PSScriptRoot/src/dashboard.cpp"
if ($LASTEXITCODE) { throw 'Falló la compilación PowerPC.' }
& "$sdkRoot/bin/win32/link.exe" /nologo /subsystem:xbox /machine:ppcbe /entry:mainCRTStartup /base:0x82000000 /stack:0x40000 /out:"$output/dashboard.exe" /libpath:"$sdkRoot/lib/xbox" "$output/dashboard.obj" d3d9.lib d3dx9.lib xgraphics.lib xapilib.lib xboxkrnl.lib xmcore.lib
if ($LASTEXITCODE) { throw 'Falló el enlace PowerPC.' }
& "$sdkRoot/bin/win32/imagexex.exe" /nologo /in:"$output/dashboard.exe" /out:"$payload/default.xex" /titleid:0x454D3601
if ($LASTEXITCODE) { throw 'Falló ImageXEX.' }
& "$sdkRoot/bin/win32/imagexex.exe" /dump "$payload/default.xex" > "$output/xex-audit.txt"
if ($LASTEXITCODE) { throw 'Falló la inspección XEX.' }
Get-FileHash "$payload/default.xex" -Algorithm SHA256
$tracked = Join-Path $PSScriptRoot 'package'
New-Item -ItemType Directory -Force $tracked | Out-Null
Copy-Item -LiteralPath "$payload/default.xex" -Destination "$tracked/default.xex" -Force
Copy-Item -LiteralPath "$payload/font.png" -Destination "$tracked/font.png" -Force
Write-Output "Dashboard actualizado en $tracked. GitHub Desktop mostrará los archivos modificados."
