#!/bin/python3
from PIL import Image

LCD_WIDTH = 320
LCD_HEIGHT = 240
LOGO_COLORS = 3
PARALLEL_LINES = 60

image = Image.open("data/logo.png")

scale = min(LCD_WIDTH / image.width, LCD_HEIGHT / image.height)

scaled_width = int(image.width * scale)
scaled_height = int(image.height * scale)

x = (LCD_WIDTH - scaled_width) // 2
y = (LCD_HEIGHT - scaled_height) // 2

background = Image.new("RGB", (LCD_WIDTH, LCD_HEIGHT))
background.paste(image.resize((scaled_width, scaled_height)), (x, y))
quantized = background.quantize(LOGO_COLORS)
palette = quantized.getpalette()
assert len(palette) == LOGO_COLORS * 3

with open("data/logo.bin", "wb") as f:
    for i in range(LOGO_COLORS):
        r = palette[i * 3]
        g = palette[i * 3 + 1]
        b = palette[i * 3 + 2]
        f.write(
            bytearray(
                [
                    (r & 0b11111000) | ((g >> 5) & 0b00000111),
                    ((g << 3) & 0b11100000) | ((b >> 3) & 0b00011111),
                ]
            )
        )
    color = None
    length = 0
    for i, pixel in enumerate(quantized.getdata()):
        if i == 0:
            color = pixel
            length = 1
        elif pixel != color or i % (PARALLEL_LINES * LCD_WIDTH) == 0 or length >= 255:
            f.write(bytearray([color, length]))
            length = 1
            color = pixel
        else:
            length += 1
    f.write(bytearray([color, length]))
