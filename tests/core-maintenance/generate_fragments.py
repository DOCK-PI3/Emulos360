"""Copy exact production bodies; test fixtures replace only host dependencies."""
import hashlib
import json
from pathlib import Path
import re
import sys

core, output = map(Path, sys.argv[1:])
source = core / 'src/xenia/kernel/xam/xam_net.cc'
text = source.read_text(encoding='utf-8')

def block(start, end, include_end=False, after=None):
    first = text.index(start, text.index(after) if after else 0)
    last = text.index(end, first)
    return text[first:last + (len(end) if include_end else 0)]

fragments = {
    'fd.inc': block('struct x_fd_set {', '};', True) + '\n' +
              block('bool verify_x_fd_set(', '\nint_result_t NetDll_select_entry'),
    'qos_types.inc': block('enum XNET_XNQOSINFO {', '};', True) + '\n' +
                     block('struct XNQOSINFO {', '};', True),
    'qos_copy.inc': block('      if (chunk.http_code == HTTP_STATUS_CODE::HTTP_OK) {',
                         '      // 415607DD and 415607D4 expect probes count,'),
    'bind_upnp.inc': block('  const auto upnp = kernel_state()->emulator()->GetUPnP();',
                          '  if (cvars::logging) {', after='dword_result_t NetDll_bind_entry('),
}
fd_size = re.search(r'^#define X_FD_SETSIZE \d+$',
                    (core / 'src/xenia/xbox.h').read_text(encoding='utf-8'), re.M)
if not fd_size:
    raise SystemExit('Missing actual socket array capacity')
fragments['fd_size.inc'] = fd_size.group(0) + '\n'
output.mkdir(parents=True, exist_ok=True)
for name, body in fragments.items():
    (output / name).write_text(body, encoding='utf-8')
(output / 'fragment-evidence.json').write_text(json.dumps({
    'source': str(source.resolve()),
    'sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
    'fragments': {name: hashlib.sha256(body.encode()).hexdigest()
                  for name, body in fragments.items()},
    'scope': 'Actual source fragments with allocator/object-table/UPnP fixtures; not full exports or gameplay',
}, indent=2) + '\n', encoding='utf-8')
