"""Rasterize the locally installed UI font for this personal dashboard build."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
import os, sys

dest = Path(sys.argv[1])
dest.mkdir(parents=True, exist_ok=True)
font = ImageFont.truetype(str(Path(os.environ['WINDIR']) / 'Fonts/segoeui.ttf'), 36)
atlas = Image.new('RGBA', (1024, 1024))
draw = ImageDraw.Draw(atlas)
widths = []
for code in range(32, 256):
    char = bytes([code]).decode('latin1')
    col, row = (code - 32) % 16, (code - 32) // 16
    # A clear gutter stops neighbouring glyphs bleeding into small text.
    draw.text((col * 64 + 2, row * 64 + 2), char, font=font, fill='white')
    widths.append(round(draw.textlength(char, font=font), 3))
atlas.save(dest / 'font.png')
(dest.parent / 'font_widths.h').write_text('static const float kWidths[224] = {' + ','.join(str(w)+'f' for w in widths) + '};\n')
