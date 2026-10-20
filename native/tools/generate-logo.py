"""Regenerate the fixed A identity for desktop/file association assets (Pillow)."""
from pathlib import Path
from PIL import Image, ImageDraw
import math

ASSETS = Path(__file__).resolve().parents[1] / 'assets'

def icon(size):
    scale = size * 4 / 256
    image = Image.new('RGBA', (size * 4, size * 4))
    draw = ImageDraw.Draw(image)
    draw.rounded_rectangle((0, 0, size*4-1, size*4-1), radius=56*scale, fill='#FAF9F6')
    stroke = 32 if size <= 24 else 24
    def point(radius, degrees):
        a = math.radians(degrees)
        return ((128 + radius*math.cos(a))*scale, (128 + radius*math.sin(a))*scale)
    angles = [-21 + 312*i/1200 for i in range(1201)]
    polygon = [point(84+stroke/2, a) for a in angles] + [point(84-stroke/2, a) for a in reversed(angles)]
    draw.polygon(polygon, fill='#486451')
    for angle in (-21, 291):
        x, y = point(84, angle)
        half = stroke*scale/2
        draw.ellipse((x-half,y-half,x+half,y+half), fill='#486451')
    return image.resize((size,size), Image.Resampling.LANCZOS)

def main():
    start = (128+84*math.cos(math.radians(-21)), 128+84*math.sin(math.radians(-21)))
    end = (128+84*math.cos(math.radians(291)), 128+84*math.sin(math.radians(291)))
    (ASSETS/'zenzip.svg').write_text(
        '<svg xmlns="http://www.w3.org/2000/svg" width="256" height="256" viewBox="0 0 256 256">'
        '<rect width="256" height="256" rx="56" fill="#FAF9F6"/>'
        f'<path d="M{start[0]:.6f} {start[1]:.6f} A84 84 0 1 1 {end[0]:.6f} {end[1]:.6f}" '
        'fill="none" stroke="#486451" stroke-width="24" stroke-linecap="round"/></svg>\n', encoding='utf-8')
    icon(1024).save(ASSETS/'zenzip.png')
    sizes = [16,24,32,48,64,128,256]
    frames = [icon(size) for size in sizes]
    frames[-1].save(ASSETS/'zenzip.ico',format='ICO',append_images=frames[:-1],sizes=[(s,s) for s in sizes])
    with Image.open(ASSETS/'zenzip.ico') as result:
        assert result.ico.sizes() == {(s,s) for s in sizes}

if __name__ == '__main__':
    main()
