from PIL import Image
import sys
import struct

img = Image.open("demofont.gif")

img = img.convert("P", palette=Image.ADAPTIVE, colors=16)

cdata = bytearray()

for x in xrange(0,50):
	xi = (x%20)*16
	yi = (x/20)*16
	img2 = img.crop((xi, yi, xi+16, yi+16))
	for py in xrange(0,16):
		for px in xrange(0,8):
			dbyte = (img2.getpixel((px*2, py)) & 0x0F) << 4
			dbyte += (img2.getpixel(((px*2)+1, py)) & 0x0F)
			cdata += struct.pack('B',dbyte)
			
pal = img.getpalette()
cdata = bytearray(pal[0:48]) + cdata

f = open("demofont.bin", 'wb')
f.write(cdata)
f.close()
