#!/usr/bin/env python3
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
  RS = chr(data[0])

  data = file.read(8)
  timestamp = struct.unpack('L', data)[0]

  data = file.read(12)
  hex_data = ['0x{:02X}, '.format(x) for x in data]
  print(f"{timestamp} - {RS}: {''.join(hex_data)}")

file.close()

