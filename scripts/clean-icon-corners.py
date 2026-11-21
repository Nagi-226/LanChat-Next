"""Remove only the edge-connected checkerboard matte, keeping the original icon art."""
from collections import deque
from pathlib import Path
from PIL import Image

root = Path(__file__).resolve().parents[1]
icons = root / "src/client-qt/resources/icons"
image = Image.open(icons / "lanchat-1024.png").convert("RGBA")
width, height = image.size
pixels = list(image.getdata())
eligible = bytearray(1 if a == 0 or (min(r, g, b) > 140 and max(r, g, b) - min(r, g, b) < 80)
                     else 0 for r, g, b, a in pixels)
removed = bytearray(width * height)
queue = deque()
def add(index):
    if eligible[index] and not removed[index]:
        removed[index] = 1
        queue.append(index)
for x in range(width):
    add(x)
    add((height - 1) * width + x)
for y in range(height):
    add(y * width)
    add(y * width + width - 1)
# 原图右下边缘还有几粒同色白底残片，与内部聊天图案分离。
for y in range(int(height * 0.85), height):
    for x in range(int(width * 0.75), width):
        add(y * width + x)
while queue:
    index = queue.popleft()
    x, y = index % width, index // width
    if x: add(index - 1)
    if x + 1 < width: add(index + 1)
    if y: add(index - width)
    if y + 1 < height: add(index + width)
image.putdata([(r, g, b, 0) if removed[index] else (r, g, b, a)
               for index, (r, g, b, a) in enumerate(pixels)])
image.save(icons / "lanchat-1024.png")
small = image.resize((256, 256), Image.Resampling.LANCZOS)
small.save(icons / "lanchat-256.png")
small.save(icons / "lanchat.ico", sizes=[(n, n) for n in (16, 24, 32, 48, 64, 128, 256)])
assert small.getpixel((0, 0))[3] == 0
assert small.getpixel((128, 80))[3] == 255
print(f"ICON PASS: removed {sum(removed)} matte pixels, transparent corners, preserved interior")
