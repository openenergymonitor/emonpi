#!/usr/bin/env python
import lcddriver
import time
import sys
import smbus

# Find LCD (alternative to i2cdetect)
bus = smbus.SMBus(1)
for address in ['0x27','0x3f']:
    try:
        bus.read_byte(int(address,16))
        break
    except Exception:
        address=False

if not address:
    sys.exit()

lcd = lcddriver.lcd(int(address, 16))
lcd.backlight = 1
lcd.lcd_display_string("Updating........", 1)
lcd.lcd_display_string("DO NOT UNPLUG!  ", 2)
