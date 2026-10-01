"""Render the two original six-second Emulos360 startup films.

Requires Pillow, NumPy and FFmpeg (the local imageio-ffmpeg wheel is supported).
The encoded films are application assets; FFmpeg is only a build-time tool.
"""

from __future__ import annotations

import math
import shutil
import subprocess
import sys
import wave
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont


ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "assets" / "intro"
WIDTH, HEIGHT, FPS, SECONDS = 1280, 720, 24, 6
THEMES = {
    "aurora": {"dark": (4, 11, 30), "light": (15, 55, 108), "accent": (64, 192, 255), "second": (113, 121, 255)},
    "nova": {"dark": (27, 7, 31), "light": (94, 26, 69), "accent": (255, 125, 101), "second": (255, 92, 181)},
}


def font(size: int, light: bool = False) -> ImageFont.FreeTypeFont:
    name = "segoeuil.ttf" if light else "segoeuib.ttf"
    return ImageFont.truetype(str(Path("C:/Windows/Fonts") / name), size)


def background(theme: dict) -> Image.Image:
    y, x = np.mgrid[0:HEIGHT, 0:WIDTH]
    vertical = (y / HEIGHT)[..., None]
    radial = np.exp(-(((x - WIDTH * .5) / 550) ** 2 + ((y - HEIGHT * .42) / 390) ** 2) * 1.7)[..., None]
    dark = np.asarray(theme["dark"], dtype=np.float32)
    light = np.asarray(theme["light"], dtype=np.float32)
    rgb = dark + (light - dark) * (.13 + .65 * radial) + (8 * vertical)
    return Image.fromarray(np.uint8(np.clip(rgb, 0, 255)), "RGB").convert("RGBA")


def sphere(theme: dict) -> Image.Image:
    size = 440
    y, x = np.mgrid[0:size, 0:size]
    nx = (x - size / 2) / (size / 2)
    ny = (y - size / 2) / (size / 2)
    radius = np.sqrt(nx * nx + ny * ny)
    shine = np.exp(-((nx + .35) ** 2 + (ny + .4) ** 2) * 3)
    rim = np.clip((radius - .58) * 2.3, 0, 1)
    tone = 140 + 94 * shine - 77 * rim
    accent = np.asarray(theme["accent"], dtype=np.float32)
    rgb = np.stack((tone, tone + 5, tone + 13), axis=2)
    rgb = rgb * .83 + accent[None, None, :] * .17
    alpha = np.uint8(np.clip((1.01 - radius) * 180, 0, 255))
    orb = Image.fromarray(np.dstack((np.uint8(np.clip(rgb, 0, 255)), alpha)), "RGBA")
    draw = ImageDraw.Draw(orb, "RGBA")
    draw.ellipse((18, 18, size - 18, size - 18), outline=(235, 247, 255, 165), width=3)
    draw.ellipse((36, 36, size - 36, size - 36), outline=(*theme["accent"], 68), width=7)
    letter = font(265)
    box = draw.textbbox((0, 0), "E", font=letter)
    pos = ((size - box[2]) / 2 - box[0], (size - box[3]) / 2 - box[1] - 13)
    draw.text((pos[0] + 7, pos[1] + 9), "E", font=letter, fill=(15, 31, 58, 105))
    draw.text(pos, "E", font=letter, fill=(249, 252, 255, 225), stroke_width=1, stroke_fill=(205, 233, 248, 140))
    return orb


def smooth(a: float, b: float, value: float) -> float:
    p = max(0.0, min(1.0, (value - a) / (b - a)))
    return p * p * (3 - 2 * p)


def draw_frame(t: float, theme: dict, base: Image.Image, orb: Image.Image) -> Image.Image:
    image = base.copy()
    accent, second = theme["accent"], theme["second"]
    fade_in = smooth(.4, 1.5, t)
    fade_out = 1 - smooth(5.2, 6, t)
    glow = Image.new("RGBA", (WIDTH, HEIGHT))
    gd = ImageDraw.Draw(glow, "RGBA")
    cx, cy = WIDTH // 2, 288
    phase = t * 1.4
    for i in range(4):
        radius = 190 + i * 44 + 10 * math.sin(phase + i)
        opacity = int((72 - i * 11) * fade_in * fade_out)
        gd.ellipse((cx - radius, cy - radius, cx + radius, cy + radius), outline=(*accent, opacity), width=4)
    sweep_x = int(-300 + smooth(.25, 3.0, t) * 1880)
    gd.line((sweep_x - 600, cy + 80, sweep_x + 60, cy - 75), fill=(*second, int(115 * fade_out)), width=12)
    image = Image.alpha_composite(image, glow.filter(ImageFilter.GaussianBlur(19)))
    draw = ImageDraw.Draw(image, "RGBA")
    for i in range(42):
        a = i * 2.39996 + phase * (.12 + i % 3 * .03)
        r = 210 + (i * 73 % 490) * (1 - .27 * smooth(.1, 1.8, t))
        px = cx + math.cos(a) * r
        py = cy + math.sin(a) * r * .55
        strength = int((27 + i % 5 * 12) * fade_out)
        draw.ellipse((px - 2, py - 2, px + 2, py + 2), fill=(*accent, strength))
    ring_alpha = int(175 * fade_in * fade_out)
    for i, color in enumerate((second, accent)):
        width = 555 + i * 35
        height = 170 + i * 10
        draw.arc((cx - width // 2, cy - height // 2, cx + width // 2, cy + height // 2),
                 start=-34 + t * 26 + i * 135, end=134 + t * 26 + i * 135,
                 fill=(*color, ring_alpha), width=3)
    scale = .25 + .75 * smooth(.75, 2.1, t)
    size = max(1, int(440 * scale))
    face = orb.resize((size, size), Image.Resampling.LANCZOS)
    face.putalpha(face.getchannel("A").point(lambda a: int(a * fade_in * fade_out)))
    image.alpha_composite(face, (cx - size // 2, cy - size // 2))
    draw = ImageDraw.Draw(image, "RGBA")
    title_alpha = int(255 * smooth(2.05, 3.1, t) * fade_out)
    title_font = font(68, light=True)
    title = "Emulos360"
    title_width = draw.textlength(title, font=title_font)
    draw.text(((WIDTH - title_width) / 2, 522), title, font=title_font,
              fill=(248, 250, 255, title_alpha))
    subtitle = "UNA GENERACIÓN. TU COLECCIÓN."
    sub_font = font(18, light=True)
    sub_width = draw.textlength(subtitle, font=sub_font)
    draw.text(((WIDTH - sub_width) / 2, 618), subtitle, font=sub_font,
              fill=(*accent, int(title_alpha * .78)))
    if t > 5.45:
        image = Image.blend(image, Image.new("RGBA", image.size, (12, 17, 21, 255)), smooth(5.45, 6, t))
    return image.convert("RGB")


def make_audio(path: Path) -> None:
    rate = 44100
    seconds = np.arange(rate * SECONDS, dtype=np.float64) / rate
    mix = np.zeros_like(seconds)
    for start, notes in ((1.35, (392, 587)), (2.12, (494, 740)), (2.84, (587, 880))):
        p = seconds - start
        active = (p >= 0) & (p < 2.3)
        envelope = np.where(active, np.minimum(1, np.maximum(0, p) * 16) * np.exp(-np.maximum(0, p) * 2.3), 0)
        for note in notes:
            mix += .10 * np.sin(2 * np.pi * note * p) * envelope
    audio = np.int16(np.clip(mix, -1, 1) * 32767)
    with wave.open(str(path), "wb") as wav:
        wav.setnchannels(1)
        wav.setsampwidth(2)
        wav.setframerate(rate)
        wav.writeframes(audio.tobytes())


def render(name: str, theme: dict, ffmpeg: str, audio: Path) -> None:
    base, orb = background(theme), sphere(theme)
    movie = OUTPUT / f"{name}.mp4"
    cmd = [ffmpeg, "-y", "-loglevel", "error", "-f", "rawvideo", "-pix_fmt", "rgb24",
           "-s", f"{WIDTH}x{HEIGHT}", "-r", str(FPS), "-i", "pipe:0", "-i", str(audio),
           "-c:v", "libx264", "-preset", "fast", "-crf", "20", "-pix_fmt", "yuv420p",
           "-c:a", "aac", "-b:a", "96k", "-t", str(SECONDS), "-movflags", "+faststart", str(movie)]
    process = subprocess.Popen(cmd, stdin=subprocess.PIPE, stderr=subprocess.PIPE)
    try:
        for frame_number in range(FPS * SECONDS):
            frame = draw_frame(frame_number / FPS, theme, base, orb)
            if frame_number == FPS * 4:
                frame.save(OUTPUT / f"{name}.png", optimize=True)
            process.stdin.write(frame.tobytes())
        process.stdin.close()
        error = process.stderr.read().decode(errors="replace")
        if process.wait() != 0:
            raise RuntimeError(error)
    finally:
        if process.poll() is None:
            process.kill()
    print(movie, movie.stat().st_size)


def main() -> None:
    OUTPUT.mkdir(parents=True, exist_ok=True)
    sys.path.insert(0, str(ROOT / ".tools" / "intro-render"))
    try:
        import imageio_ffmpeg
        ffmpeg = imageio_ffmpeg.get_ffmpeg_exe()
    except ImportError:
        ffmpeg = shutil.which("ffmpeg")
    if not ffmpeg:
        raise SystemExit("FFmpeg is required to render the intro films")
    audio = OUTPUT / "intro-audio.wav"
    make_audio(audio)
    try:
        for name, theme in THEMES.items():
            render(name, theme, ffmpeg, audio)
    finally:
        audio.unlink(missing_ok=True)


if __name__ == "__main__":
    main()
