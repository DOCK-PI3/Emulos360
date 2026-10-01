"""Only checks Emulos360's two-device extension, without booting any game."""
import pathlib, subprocess, tempfile, tomllib, shutil
project=pathlib.Path(__file__).resolve().parents[1]
core=project/'engine/xenia/build/bin/Windows/Release/Emulos360-core.exe'
with tempfile.TemporaryDirectory(prefix='emulos-storage-',dir=project/'local') as folder:
    root=pathlib.Path(folder)
    (root/'content').mkdir(); (root/'external').mkdir()
    (root/'content/.emulos-storage-check').touch()
    output=root/'result.toml'
    command=[str(core),f'--storage_root={root}',f'--content_root={root / "content"}',
             f'--emulos_external_content_root={root / "external"}',
             '--emulos_profile_command=storage-check',f'--emulos_profile_output={output}',
             '--network_mode=0','--upnp=false','--discord=false','--auto_check_updates=false']
    try:
        subprocess.run(command,cwd=root,check=True,timeout=60,capture_output=True)
        result=tomllib.loads(output.read_text(encoding='utf-8'))
        assert result['ok'],result
    except Exception:
        for logfile in root.glob('*.log'): shutil.copy2(logfile,project/'local'/('storage-failed-'+logfile.name))
        raise
    print('PASS: actual ContentManager create/enumerate/reopen isolates both devices; disconnected external fails without fallback. No game launched.')
