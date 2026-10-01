"""Compare the UI catalog to flags actually exported by our compiled core."""
import json, pathlib, subprocess, tempfile, tomllib
project=pathlib.Path(__file__).resolve().parents[1]
core=project/'engine/xenia/build/bin/Windows/Release/Emulos360-core.exe'
def flatten(table, prefix=''):
    result={}
    for key,value in table.items():
        name=prefix+key
        if isinstance(value,dict): result.update(flatten(value,name+'.'))
        else: result[name]=value
    return result
with tempfile.TemporaryDirectory(prefix='emulos-catalog-',dir=project/'local') as folder:
    root=pathlib.Path(folder); config=root/'engine.toml'
    config.write_text('',encoding='utf-8')
    subprocess.run([str(core),f'--storage_root={root}',f'--content_root={root / "content"}',
                    f'--config={config}','--emulos_profile_command=catalog',
                    f'--emulos_profile_output={root / "result.toml"}',
                    '--network_mode=0','--upnp=false','--discord=false'],cwd=root,check=True,timeout=60,capture_output=True)
    actual=flatten(tomllib.loads(config.read_text(encoding='utf-8-sig')))
    catalog=json.loads((project/'app/settings-catalog.json').read_text(encoding='utf-8'))
    expected={e['key'] for e in catalog['entries']}
    missing=sorted(actual.keys()-expected); extra=sorted(expected-actual.keys())
    report=dict(actualCount=len(actual),catalogCount=len(expected),missing=missing,extra=extra)
    (project/'local/netplay-catalog-report.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print(json.dumps(report))
    assert not missing and not extra, 'Catalog differs from compiled core'
