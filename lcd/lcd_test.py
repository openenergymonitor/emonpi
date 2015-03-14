#!/usr/bin/env python

import lcddriver
import time

lcd = lcddriver.lcd()


lcd.lcd_display_string("emonPi", 1)
lcd.lcd_display_string("test 123",2)  
time.sleep(5)
lcd.lcd_display_string("Backlight off", 1)
lcd.lcd_display_string("in 5 seconds..",2)
time.sleep(5)
lcd.backlight(0)
time.sleep(5)
#lcd.backlight(1)
lcd.lcd_clear()
lcd.lcd_display_string("Backlight on", 1)
lcd.lcd_display_string("end of test", 2)




