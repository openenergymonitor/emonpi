#!/usr/bin/env python
import sys
import smbus
import emonPiLCD1
import emonPiLCD2

# Find LCD (alternative to i2cdetect)
bus = smbus.SMBus(1)
for address in ['0x27','0x3f','0x3c']:
    try:
        print(bus.read_byte(int(address,16)))
        break
    except Exception:
        address=False

print(address)
