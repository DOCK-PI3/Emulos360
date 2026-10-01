"""Import the installed XDK's avatar rig into a private local resource folder.

No SDK code or DLL is linked into Emulos360. The resulting data stays outside
Git/public packages, like the user's original Xbox resource catalog.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("destination", type=Path)
    args = parser.parse_args()
    sdk = Path(os.environ["XEDK"])
    if not (sdk / "bin/win32/Microsoft.XboxLive.Avatars.dll").is_file():
        raise SystemExit("The original installed XDK avatar tools are required for this private import.")
    script = r"""
$ErrorActionPreference='Stop'
[void][Reflection.Assembly]::LoadFrom((Join-Path $env:XEDK 'bin/win32/Microsoft.XboxLive.Avatars.Mathlib.dll'))
$assembly=[Reflection.Assembly]::LoadFrom((Join-Path $env:XEDK 'bin/win32/Microsoft.XboxLive.Avatars.dll'))
$type=$assembly.GetType('Microsoft.XboxLive.Avatars.Internal.Version1.EmbeddedSkeleton')
$method=$type.GetMethod('GetEmbeddedSkeleton',[Reflection.BindingFlags]'Public,NonPublic,Static')
$parameters=$method.GetParameters()
$rig=$method.Invoke($null,@([Enum]::ToObject($parameters[0].ParameterType,1),[Enum]::ToObject($parameters[1].ParameterType,2)))
$rig.Joints | ConvertTo-Json -Depth 7 -Compress
"""
    powershell = Path(os.environ["WINDIR"]) / "SysWOW64/WindowsPowerShell/v1.0/powershell.exe"
    joints = json.loads(subprocess.check_output([str(powershell), "-NoProfile", "-Command", script], text=True))
    if len(joints) != 71:
        raise SystemExit("Unrecognized original rig.")
    result = bytearray(b"EMUSKEL1" + struct.pack("<I", len(joints)))
    for i, joint in enumerate(joints):
        parent = joint["Parent"] if i else -1
        if i and not 0 <= parent < i:
            raise SystemExit("Invalid joint hierarchy.")
        values = []
        for field, axes in [(joint["BindPosition"], "XYZ"), (joint["BindRotation"], "XYZW"),
                            (joint["Local"]["position"], "XYZ"), (joint["Local"]["rotation"], "XYZW"),
                            (joint["Local"]["scale"], "XYZ")]:
            values.extend(field[axis] for axis in axes)
        result.extend(struct.pack("<i17f", parent, *values))
    args.destination.mkdir(parents=True, exist_ok=True)
    output = args.destination / "avatar-skeleton.bin"
    if output.exists():
        if output.read_bytes() != result:
            raise SystemExit("Destination contains a different rig; no file overwritten.")
    else:
        with output.open("xb") as stream:
            stream.write(result)
    print(json.dumps({"file": str(output), "sha256": hashlib.sha256(result).hexdigest(), "joints": len(joints), "source": "Locally installed XDK 21256.3, Natal rig, right-handed"}))


if __name__ == "__main__":
    main()
