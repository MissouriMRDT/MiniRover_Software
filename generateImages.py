#!/bin/python3
import os
from PIL import Image


def get_define_value(header: str, name: str) -> str:
    location = header.find(f"#define {name} ")
    assert location >= 0
    end = header.find("\n", location)
    assert end > location + len(name) + 9
    return header[location + len(name) + 9 : end]


# Get expected image information from header file.
with open("include/tft.h", "r") as f:
    tft_h = f.read()
    LCD_WIDTH = int(get_define_value(tft_h, "LCD_WIDTH"))
    LCD_HEIGHT = int(get_define_value(tft_h, "LCD_HEIGHT"))
    PALATTE_SIZE = int(get_define_value(tft_h, "PALATTE_SIZE"))
    PARALLEL_LINES = int(get_define_value(tft_h, "PARALLEL_LINES"))

image_names = sorted(os.listdir("images"))
with open("data/images.js", "w") as f:
    f.write(
        f"const IMAGE_NAMES = [{", ".join([f'"{image_name}"' for image_name in image_names])}];"
    )

with open("data/images.bin", "wb") as f:
    for image_name in image_names:
        image = Image.open(f"images/{image_name}")

        scale = min(LCD_WIDTH / image.width, LCD_HEIGHT / image.height)

        scaled_width = int(image.width * scale)
        scaled_height = int(image.height * scale)

        x = (LCD_WIDTH - scaled_width) // 2
        y = (LCD_HEIGHT - scaled_height) // 2

        background = Image.new("RGB", (LCD_WIDTH, LCD_HEIGHT))
        background.paste(image.resize((scaled_width, scaled_height)), (x, y))
        quantized = background.quantize(PALATTE_SIZE)
        palette = quantized.getpalette()
        assert len(palette) == PALATTE_SIZE * 3

        for i in range(PALATTE_SIZE):
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
        data = quantized.getdata()
        for i in range(0, LCD_WIDTH * LCD_HEIGHT, 4):
            f.write(
                bytearray(
                    [
                        data[i]
                        | (data[i + 1] << 2)
                        | (data[i + 2] << 4)
                        | (data[i + 3] << 6)
                    ]
                )
            )
