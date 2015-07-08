#!/usr/bin/env python
import serial
import array

#=========================================================
PORT = '/dev/ttyUSB1'
FILE = 'build/ch.bin'

RAM_ADDR    = 0x10000300
PAGE_SIZE   = 0x40
SECTOR_SIZE = 0x400
FLASH_SIZE  = 0x4000

#=========================================================
ser = serial.Serial( PORT, 115200, timeout=1)

data = array.array('B')
f = file( FILE, 'rb')
try:
  data.fromfile(f, FLASH_SIZE)
except:
  pass
f.close()

## pad out to next whole page
data.fromstring( chr(0xff)*(PAGE_SIZE - (data.buffer_info()[1]%PAGE_SIZE)) )

#=========================================================
## fix-up LPC boot checksum
csum = 0;
for i in range(7):
  csum = csum + \
  (data[(i*4)]      ) + \
  (data[(i*4)+1]<<8 ) + \
  (data[(i*4)+2]<<16) + \
  (data[(i*4)+3]<<24); \

csum = -csum
data[28] = csum     & 0xff
data[29] = csum>>8  & 0xff
data[30] = csum>>16 & 0xff
data[31] = csum>>24 & 0xff

#=========================================================
##
ser.write('?')
resp = ser.readline()
if resp.strip() <> 'Synchronized':
  print 'No Response "?"'
  exit(1)

ser.write('Synchronized\r\n')
resp = ser.readline()
resp = ser.readline()
if resp.strip() <> 'OK':
  print 'Not Synchronized'
  exit(1)
  
ser.write('12000\r\n')
resp = ser.readline()
resp = ser.readline()
if resp.strip() <> 'OK':
  print 'No Response "12000"'
  exit(1)

ser.write('A 0\r\n')
resp = ser.readline()
resp = ser.readline()
if resp.strip() <> '0':
  print 'Error Response "A"', resp
  exit(1)

ser.write('J\r\n')
resp = ser.readline()
if resp.strip() <> '0':
  print 'Error Response "J"', resp
  exit(1)
resp = ser.readline()
print 'Device ID: ', hex(int(resp))

ser.write('U 23130\r\n')
resp = ser.readline()
if resp.strip() <> '0':
  print 'Error Response "U"', resp
  exit(1)


## Erase whole device
ser.write('P 0 7\r\n')
resp = ser.readline()
if resp.strip() <> '0':
  print 'Error Response "P"', resp
  exit(1)

ser.write('E 0 7\r\n')
resp = ser.readline()
if resp.strip() <> '0':
  print 'Error Response "P"', resp
  exit(1)


#=========================================================
address = 0

while data.buffer_info()[1]:
  
  ser.write( "W %d %d\r\n"%(RAM_ADDR, PAGE_SIZE) )
  resp = ser.readline()
  if resp.strip() <> '0':
    print 'Error Response "W"', resp
    exit(1)

  for i in range(PAGE_SIZE):
    ser.write( chr(data.pop(0)) )
    
  #print('P %x %x\r\n'%( address/SECTOR_SIZE, address/SECTOR_SIZE ))
  #print('C %x %x 0xff\r\n'%( address, RAM_ADDR ))

  ## Program page
  ser.write('P %d %d\r\n'%( address/SECTOR_SIZE, address/SECTOR_SIZE ))
  resp = ser.readline()
  if resp.strip() <> '0':
    print 'Error Response "P"', resp
    exit(1)

  ser.write( 'C %d %d %d\r\n'%(address, RAM_ADDR, PAGE_SIZE) )
  resp = ser.readline()
  if resp.strip() <> '0':
    print 'Error Response "C"', resp
    exit(1)

  print '.',
  address = address + PAGE_SIZE
  if (address%SECTOR_SIZE) == 0:
    print ''

#=========================================================
#=========================================================



