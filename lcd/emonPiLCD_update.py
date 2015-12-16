#!/usr/bin/env python

import lcddriver
import time
import sys

lcd = lcddriver.lcd()

lcd.lcd_display_string("Updating........", 1)
lcd.lcd_display_string("DO NOT UNPLUG!  ",2)  
time.sleep(1)
sys.exit()




