import lcddriver
from subprocess import *
from time import sleep, strftime
from datetime import datetime

lcd = lcddriver.lcd()


lcd.lcd_display_string("emonPi", 1)
lcd.lcd_display_string("IP clock test",2)  
sleep(1)
 
wlan0 = "ip addr show wlan0 | grep inet | awk '{print $2}' | cut -d/ -f1 | head -n1"
eth0 = "ip addr show eth0 | grep inet | awk '{print $2}' | cut -d/ -f1 | head -n1"
 

p = Popen(eth0, shell=True, stdout=PIPE)
IP = p.communicate()[0]
if IP == "":
	p = Popen(wlan0, shell=True, stdout=PIPE)
	IP = p.communicate()[0]
print IP

 
while 1:
        #lcd.lcd_clear()
        ipaddr = run_cmd(cmd)
        lcd.lcd_display_string(datetime.now().strftime('%b %d  %H:%M:%S\n'),1)
        lcd.lcd_display_string('IP %s' % ( IP ),2)
        sleep(2)




