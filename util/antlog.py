#!/usr/bin/python

#
# Dump timestamps & formatted 12 byte ANT messages from a GC antlog.raw file;
#
# data[0]    sync byte - always 0xA4
# data[1]    message length
# data[2]    message id - 0x4E is broadcast data
# data[3-11] message bytes
#             - for broadcast data, data[3] is the channel number
#

import sys
import os
import struct

file = open(sys.argv[1], 'rb')
for line in range(0, os.path.getsize(sys.argv[1]), 21):

  data = file.read(1)
  RS = data[0];

  data = file.read(8)
  timestamp = struct.unpack('L', data)[0]

  data = file.read(12)
  print str(timestamp) + ' - ' \
      + RS  + ': ' \
      + '0x' + (data[0]).encode('hex').upper()  + ', ' \
      + '0x' + (data[1]).encode('hex').upper()  + ', ' \
      + '0x' + (data[2]).encode('hex').upper()  + ', ' \
      + '0x' + (data[3]).encode('hex').upper()  + ', ' \
      + '0x' + (data[4]).encode('hex').upper()  + ', ' \
      + '0x' + (data[5]).encode('hex').upper()  + ', ' \
      + '0x' + (data[6]).encode('hex').upper()  + ', ' \
      + '0x' + (data[7]).encode('hex').upper()  + ', ' \
      + '0x' + (data[8]).encode('hex').upper()  + ', ' \
      + '0x' + (data[9]).encode('hex').upper()  + ', ' \
      + '0x' + (data[10]).encode('hex').upper() + ', ' \
      + '0x' + (data[11]).encode('hex').upper()

file.close()

