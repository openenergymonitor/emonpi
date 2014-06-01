import pylcdlib
lcd = pylcdlib.lcd(0x27,1,1)  #raspberry pi ver2, (I2C address, I2C bus 512Mb Pi=1 256Mb Pi=0, reverse code 0-2)  
lcd.lcd_puts("emonPi",1) 
lcd.lcd_puts("Test LCD",2)  


