import sys
import struct

f = open('reality.ym', 'r')
data = f.read()
f.close()

rdata = bytearray()
cdata = bytearray()
diffs = bytearray()
rawdata = bytearray()
ptrs = bytearray()

startframe = (76 * struct.unpack('>H', data[26:28])[0]) + 40
stopframe = (15 * struct.unpack('>H', data[26:28])[0]) + 18
frames = struct.unpack('>I', data[12:16])[0]

print startframe

for r in xrange(0,14): #16
	ptrs += struct.pack('<H', len(cdata) + 30)
	start = 0x58 + (r * frames)
	rdata = data[start+startframe:start+stopframe+startframe]
	rawdata += rdata
	lastb = rdata[0]
	ctd = 1
	cts = 0
	for by in rdata[1:]:
		if ((by == lastb) | (ctd == 127)):
			if (ctd > 1):
				cdata += struct.pack('B', len(diffs)) + diffs
				print "Same after diff " + hex(ctd)
			diffs = ""
			ctd = 0
			cts += 1
			print "Same " + hex(struct.unpack("B", lastb)[0]) + " " + hex(cts)
		if ((by != lastb) | (cts == 127)):
			# Diff: New run or inc diff
			if (cts > 0):
				# After a run of same
				if (cts < 127): cts += 1
				cdata += struct.pack('B', cts + 0x80) + lastb
				ctd = 1
				print "Diff after same " + hex(struct.unpack("B", by)[0]) + " " + hex(cts)
			else:
				# At least 2 diffs
				if (ctd == 1):
					ctd = 2
					diffs = lastb
					print "Start diff block with " + hex(struct.unpack("B", lastb)[0])
				else:
					print "Diff " + hex(struct.unpack("B", lastb)[0]) + " " + hex(ctd)
					ctd += 1
					diffs += lastb
			cts = 0
		lastb = by
	
	if (ctd):
		if (ctd == 1):
			cdata += struct.pack('B', 1) + by
		else:
			diffs += by
			cdata += struct.pack('B', len(diffs)) + diffs

	if (cts):
		if (cts < 127): cts += 1
		cdata += struct.pack('B', cts + 0x80) + lastb

#f = open('header.bin', 'wb')
#f.write(data[0:0x0C] + struct.pack('>I', stopframe) + data[0x10:0x1C] + struct.pack('>I', 0) + data[0x20:0x59])
#f.close()

data = struct.pack('<H', stopframe) + ptrs + cdata

f = open('cut_rle.bin', 'wb')
f.write(data)
f.close()

#f = open('cut_raw.bin', 'wb')
#f.write(rawdata)
#f.close()
