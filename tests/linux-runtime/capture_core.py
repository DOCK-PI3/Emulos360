"""Capture only the core window owned by the test, never the user's desktop."""
import pathlib
import sys
from Xlib import X, Xatom, display
from Xlib.protocol import event
from PIL import Image

pid = int(sys.argv[1])
connection = display.Display()
pid_atom = connection.intern_atom('_NET_WM_PID')

def find(parent):
    for window in parent.query_tree().children:
        owner = window.get_full_property(pid_atom, Xatom.CARDINAL)
        if owner is not None and int(owner.value[0]) == pid:
            attributes = window.get_attributes()
            geometry = window.get_geometry()
            if attributes.map_state == X.IsViewable and geometry.width >= 640 and geometry.height >= 360:
                return window
        found = find(window)
        if found is not None:
            return found
    return None

window = find(connection.screen().root)
if window is None:
    raise SystemExit('Test core window not found')
size = window.get_geometry()
pixels = window.get_image(0, 0, size.width, size.height, X.ZPixmap, 0xFFFFFFFF)
image = Image.frombytes('RGB', (size.width, size.height), pixels.data, 'raw', 'BGRX')
image.save(pathlib.Path(sys.argv[2]))
print(f'Captured core PID {pid}: {size.width}x{size.height}')
if len(sys.argv) > 3 and sys.argv[3] == '--close':
    message = event.ClientMessage(window=window.id, client_type=connection.intern_atom('WM_PROTOCOLS'),
                                  data=(32, [connection.intern_atom('WM_DELETE_WINDOW'), X.CurrentTime, 0, 0, 0]))
    window.send_event(message, event_mask=0)
    connection.flush()
