from PIL import Image, ImageDraw

size = 256
im = Image.new('RGB', (size, size), '#080b10')
d = ImageDraw.Draw(im)
d.rounded_rectangle((17, 17, 239, 239), radius=52, fill='#141c27', outline='#5a6777', width=3)
d.rounded_rectangle((48, 171, 208, 181), radius=5, fill='#718398')
d.polygon([(97, 66), (97, 162), (179, 114)], fill='#f3f6f9')
im.save('app/icon.png')
