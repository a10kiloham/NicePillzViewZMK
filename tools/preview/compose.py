"""Turns the PBMs rendered by the preview binary into docs/display-preview.png."""
from PIL import Image, ImageDraw, ImageFont
import os

HERE = os.path.dirname(os.path.abspath(__file__))
BUILD = os.path.join(HERE, "build")
OUT = os.path.join(HERE, "..", "..", "docs", "display-preview.png")
SCALE = 3
SCENES = [("typical", "On battery, BT 1, Ctrl held"), ("charging", "USB, charging, Caps"),
          ("locks", "Layer 5, all locks, Ctrl+Shift"), ("unsynced", "Full battery, BT open"),
          ("inverted", "Inverted, Alt+Win held")]
BEZEL = 14
GAP = 24
LABEL_H = 22

def compose(prefix, out, pins):
    panels = []
    for name, caption in SCENES:
        im = Image.open(os.path.join(BUILD, prefix + name + ".pbm")).convert("L")
        im = im.resize((im.width * SCALE, im.height * SCALE), Image.NEAREST)
        panels.append((im, caption))

    pw = panels[0][0].width + 2 * BEZEL
    ph = panels[0][0].height + 2 * BEZEL
    W = GAP + len(panels) * (pw + GAP)
    H = GAP + ph + LABEL_H + GAP
    sheet = Image.new("RGB", (W, H), (240, 240, 236))
    draw = ImageDraw.Draw(sheet)
    try:
        font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 13)
    except OSError:
        font = ImageFont.load_default()

    x = GAP
    for im, caption in panels:
        # dark bezel like the nice!view, pins (header) drawn at the bottom
        draw.rounded_rectangle([x, GAP, x + pw, GAP + ph], radius=8, fill=(28, 28, 30))
        sheet.paste(im.convert("RGB"), (x + BEZEL, GAP + BEZEL))
        for i in range(5):
            if pins == "bottom":
                px = x + pw // 2 - 2 * 12 + i * 12
                py = GAP + ph - 5
            else:  # header pins on the right
                px = x + pw - 5
                py = GAP + ph // 2 - 2 * 12 + i * 12
            draw.ellipse([px - 3, py - 3, px + 3, py + 3], fill=(200, 170, 60))
        draw.text((x, GAP + ph + 6), caption, fill=(40, 40, 40), font=font)
        x += pw + GAP

    sheet.save(out)
    print("wrote", out)


compose("", OUT, "bottom")
compose("landscape_", os.path.join(HERE, "..", "..", "docs", "display-preview-landscape.png"), "right")
raw = Image.open(os.path.join(BUILD, "panel_raw.pbm")).convert("L")
raw.resize((raw.width * SCALE, raw.height * SCALE), Image.NEAREST).save(
    os.path.join(HERE, "..", "..", "docs", "display-panel-raw.png"))
