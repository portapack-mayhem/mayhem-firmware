import sys
import struct

cdata = bytearray()

f = open('header.bin', 'r')
hdata = f.read()
f.close()

f = open('cut_rle.bin', 'r')
cdata = f.read()
f.close()

rdata = bytearray()

frames = struct.unpack('<H', cdata[0:2])[0]

print frames

code = 0
for by in cdata[2:]:
	t = struct.unpack('B', by)[0]
	if (code == 0):
		cnt = t & 127
		if (t < 128):
			code = 1
		else:
			code = 2
	elif (code == 1):
		rdata += by
		cnt -= 1
		if (cnt == 0): code = 0
	elif (code == 2):
		for r in xrange(0, cnt):
			rdata += by
		code = 0

f = open('unpack.ym', 'wb')
f.write(hdata + rdata)
f.close()
