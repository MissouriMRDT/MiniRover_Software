# Mini Rover Software

## Operation

## Building

Place images to draw with `tft_draw_image` in the `images` folder. The image index and order in the web drop-down is the same as the sorted filenames in this folder. Run `generateImages.py` to generate `data/{images.js,images.bin}`. `generateImages.py` resizes images to `LCD_WIDTH` by `LCD_HEIGHT` (as read from `tft.h`) while preserving aspect ratio, centers them on a black background, converts them to a palatte of 4 colors, and packs them into `images.bin`. Each image is packed as 19208 bytes: a palatte of 4 2-byte colors followed by 2-bit pixels.

### Windows

1) `py -m venv venv`
2) `venv\Scripts\python -m pip install -r requirements.txt`
3) `venv\Scripts\python generateImages.py`

### Linux

1) `python3 -m venv venv`
2) `venv/bin/pip install -r requirements.txt`
3) `venv/bin/python generateImages.py`
