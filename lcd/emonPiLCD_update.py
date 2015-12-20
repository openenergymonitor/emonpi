#!/usr/bin/env python

import lcddriver
import time
import sys
import subprocess

# ------------------------------------------------------------------------------------
# Check to see if LCD is connected if not then stop here
# ------------------------------------------------------------------------------------

lcd_status = subprocess.check_output(["/home/pi/emonpi/lcd/./emonPiLCD_detect.sh", "27"])

if lcd_status.rstrip() == 'False':
    print "I2C LCD NOT DETECTED"
    sys.exit(1)
    
lcd = lcddriver.lcd()

# ------------------------------------------------------------------------------------
# Display update in progress update
# ------------------------------------------------------------------------------------

lcd.lcd_display_string("Updating........", 1)
lcd.lcd_display_string("DO NOT UNPLUG!  ",2)
time.sleep(1)
sys.exit()




