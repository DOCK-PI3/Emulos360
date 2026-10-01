"""Refresh literal CVar definitions from the pinned Netplay source, without running it.
Keeps labels and console data from the original catalog; fails on unknown defaults.
"""
import json, re, subprocess
from pathlib import Path
R = Path(__file__).resolve().parents[1]
p = R / 'app/settings-catalog.json'
catalog = json.loads(p.read_text(encoding='utf-8'))
entries = {e['name']: e for e in catalog['entries']}
literal = r'"(?:\\.|[^"\\])*"'
def string(s):
    return ''.join(json.loads(x) for x in re.findall(literal, s))
groups = {'Live':'network', 'Storage':'storage', 'Netplay':'network', 'UI':'overlay', 'General':'general', 'Profiles':'profiles', 'HID':'input', 'Logging':'logging', 'Kernel':'system'}
seen = set()
sources = R/'engine/xenia/src/xenia'
for src in sorted([*sources.rglob('*.cc'), *sources.rglob('*.cpp')]):
    text = src.read_text(encoding='utf-8')
    for m in re.finditer(r'\bDEFINE_(bool|int32|uint32|int64|uint64|double|float|string|path)\s*\(',text):
        start = m.end(); pos = start; depth = 1; quoted = False; escape = False
        while depth:
            c = text[pos]; pos += 1
            if quoted:
                if escape: escape=False
                elif c=='\\': escape=True
                elif c=='"': quoted=False
            elif c=='"': quoted=True
            elif c=='(': depth+=1
            elif c==')': depth-=1
        args=[]; begin=start; nested=0; quoted=False; escape=False
        for i in range(start,pos-1):
            c=text[i]
            if quoted:
                if escape: escape=False
                elif c=='\\': escape=True
                elif c=='"': quoted=False
            elif c=='"': quoted=True
            elif c=='(': nested+=1
            elif c==')': nested-=1
            elif c==',' and not nested: args.append(text[begin:i].strip()); begin=i+1
        args.append(text[begin:pos-1].strip())
        if len(args)!=4: continue
        name,default,desc,section=args; seen.add(name)
        section=string(section); typ=m[1]
        if not section: continue
        if name not in entries and section not in groups: continue
        try:
            if typ in ('string','path'): value=string(default) if default.startswith('"') else (_ for _ in ()).throw(ValueError())
            elif typ=='bool': value={'true':True,'false':False}[default]
            else: value=str(int(re.sub(r'[uUlL]+$','',default),0)) if 'int' in typ else str(float(default.rstrip('fF')))
        except (ValueError,KeyError):
            if name not in entries: raise ValueError(f'Unresolved {name}: {default}')
            continue
        if name in entries:
            e=entries[name]
            e.update(default=value,description=string(desc))
        else:
            entries[name]=dict(key=section+'.'+name,name=name,section=section,group=groups.get(section,'advanced'),type=typ,default=value,description=string(desc),label=name.replace('_',' '),help='',readonly=False)
# HID.Key bindings are emitted dynamically, not through DEFINE_* macros.
# Confirmed against the compiled Windows x64 export: these definitions are
# inside #if 0 / XE_PLATFORM_ANDROID and are not settings of this binary.
not_windows={'stack_size_multiplier_hack','main_xthread_stack_size_multiplier_hack','log_to_logcat'}
catalog['entries']=[e for e in entries.values() if (e['name'] in seen or e['section']=='HID.Key') and e['name'] not in not_windows]
overrides={
 'network_mode':dict(label='Modo de red',help='Sin conexión, System Link en LAN/VPN o servicio comunitario Netplay.',choices=[dict(value='0',label='Sin conexión'),dict(value='1',label='System Link / LAN'),dict(value='2',label='Netplay comunitario')]),
 'api_address':dict(label='Servidor Netplay',help='Dirección del servidor activo. El gestor del motor permite elegirlo.'),
 'api_list':dict(label='Servidores disponibles',help='Direcciones separadas por comas. Se conserva el servicio público de Netplay.'),
 'network_guid':dict(label='Interfaz de red',help='GUID de la interfaz. Déjalo vacío para selección automática; el gestor nativo permite elegir LAN o VPN.'),
 'bind_interface':dict(label='Vincular a la interfaz seleccionada'),
 'upnp':dict(label='Abrir puertos mediante UPnP',help='Solo si lo activas: solicita al router asignaciones de puertos.'),
 'emulos_external_content_root':dict(label='Carpeta del disco externo',help='Vacío: disco virtual en external-content. También puedes elegir una carpeta de tu USB o HDD en Partidas.'),
 'auto_check_updates':dict(label='Actualizador de upstream deshabilitado',readonly=True,default=False),
 'storage_selection_dialog':dict(default=True,label='Elegir almacenamiento dentro del juego'),
}
for e in catalog['entries']: e.update(overrides.get(e['name'],{}))
catalog['revision']=subprocess.check_output(['git','-C',str(R/'engine/xenia'),'rev-parse','HEAD'],text=True).strip()
catalog['upstream']='AdrianCassar/xenia-canary:netplay_canary_experimental'
p.write_text(json.dumps(catalog,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(f"{len(catalog['entries'])} settings; pinned revision {catalog['revision']}")
