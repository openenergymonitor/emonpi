#!/usr/bin/env python
import sys
import smbus

# Find LCD (alternative to i2cdetect)
bus = smbus.SMBus(1)
for address in ['0x27','0x3f','0x3c']:
    try:
        bus.read_byte(int(address,16))
        break
    except Exception:
        address=False

if address=='0x27' or address=='0x3f':
    import emonPiLCD1
    emonPiLCD1.main()
    
elif address=='0x3c':
    import emonPiLCD2
    emonPiLCD2.main()

else:
    print("Device address not recognised")
