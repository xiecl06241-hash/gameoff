"""Original graphic assets, drawn from vectors/type. No downloaded packaging."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
import random
import json

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "SourceArt/Labels"
FONT = Path("C:/Windows/Fonts/bahnschrift.ttf")
CREAM = "#f3efe1"
INK = "#172c2c"
PRODUCTS = [
    ("Water", "PURE", "SPRING WATER", "#73b8b8", "500 mL", 2.0),
    ("Milk", "DAILY", "WHOLE MILK", "#326a75", "250 mL", 3.4),
    ("Soda", "CITRA", "SPARKLING CITRUS", "#d8a346", "330 mL", 2.8),
    ("Noodles", "MIDNIGHT", "NOODLE CUP", "#b95a3f", "86 g", 4.0),
    ("Chips", "CRISP", "SEA SALT POTATO", "#aeb36a", "70 g", 3.2),
    ("Biscuits", "OAT", "BAKED BISCUITS", "#b08764", "120 g", 4.2),
    ("Can", "PANTRY", "TOMATO SOUP", "#a85443", "220 g", 3.5),
    ("Bread", "MORNING", "SOFT BAKED BREAD", "#c49d64", "90 g", 2.4),
    ("Tissue", "SOFT", "FACIAL TISSUE", "#7f9e91", "120 sheets", 2.1),
    ("Meal", "FRESH", "RICE BOWL", "#75865b", "280 g", 7.0),
    ("Sauce", "UMAMI", "SOY SAUCE", "#6e4f39", "150 mL", 4.5),
    ("Cleaner", "CLEAR", "SURFACE CLEANER", "#699ab0", "400 mL", 5.5),
]

def font(size):
    return ImageFont.truetype(str(FONT), size)

def text(draw, xy, value, size, fill=INK, anchor=None):
    draw.text(xy, value, font=font(size), fill=fill, anchor=anchor)

def label(index, item):
    key, title, subtitle, accent, amount, cost = item
    im = Image.new("RGB", (512, 512), CREAM)
    d = ImageDraw.Draw(im)
    d.rectangle((0, 0, 512, 72), fill=INK)
    text(d, (27, 19), "SHIFT / 24", 31, CREAM)
    text(d, (480, 26), "%02d" % (index + 1), 21, CREAM, "ra")
    d.rectangle((0, 96, 512, 118), fill=accent)
    # A simple proprietary geometric illustration, distinct for every SKU.
    d.ellipse((145, 145, 367, 367), fill=accent)
    for k in range(4):
        x = 180 + k * 38
        d.line((x, 198, x + 22, 310), fill=CREAM, width=7)
    text(d, (256, 335), title, 48 if len(title) < 8 else 40, INK, "ma")
    text(d, (256, 397), subtitle, 22, INK, "ma")
    d.line((27, 435, 485, 435), fill=INK, width=2)
    text(d, (27, 455), amount, 23)
    rng = random.Random(index)
    x = 315
    while x < 483:
        width = rng.choice((2, 3, 5))
        d.rectangle((x, 455, x + width, 488), fill=INK)
        x += width + rng.choice((2, 4))
    # Minute print variation, baked once and deterministic.
    im.save(OUT / (key + ".png"))

def panel(name, width, height, title, subtitle, color=INK, text_color=CREAM):
    im = Image.new("RGB", (width, height), color)
    d = ImageDraw.Draw(im)
    d.rectangle((16, 16, width - 17, height - 17), outline=text_color, width=3)
    text(d, (width//2, height*.23), title, int(height*.28), text_color, "ma")
    text(d, (width//2, height*.66), subtitle, int(height*.10), text_color, "ma")
    im.save(OUT / (name + ".png"))

def main():
    OUT.mkdir(parents=True, exist_ok=True)
    for i, p in enumerate(PRODUCTS):
        label(i, p)
    panel("Storefront", 2048, 384, "SHIFT / 24", "OPEN ALL NIGHT  /  EVERYDAY ESSENTIALS")
    panel("Welcome", 1024, 384, "GOOD EVENING", "TAKE YOUR TIME. WE ARE ALWAYS OPEN.")
    panel("Exit", 512, 192, "EXIT  >", "KEEP CLEAR", "#217c65")
    panel("Office", 512, 256, "OFFICE", "STAFF ONLY")
    panel("Warehouse", 512, 256, "STOCKROOM", "STAFF ONLY")
    panel("Restroom", 512, 256, "RESTROOM", "PLEASE KEEP CLEAN")
    panel("Notice", 512, 768, "SHIFT / 24", "STORE INFORMATION")
    panel("Poster", 768, 1024, "LATE NIGHTS", "SMALL COMFORTS  /  FRESH EVERY DAY", "#326a75")
    panel("ShiftNotice", 1024, 384, "STILL HERE", "YOUR SHIFT IS NOT OVER.", "#263b3a", "#d9c391")
    (ROOT / "SourceArt/products.json").write_text(json.dumps([
        dict(id=p[0], name=p[1], description=p[2], cost=p[5], category=i)
        for i, p in enumerate(PRODUCTS)
    ], indent=2), encoding="utf-8")
    print("ORIGINAL_LABELS_OK", len(PRODUCTS), OUT)

if __name__ == '__main__':
    main()
