import lcddriver
from subprocess import *
from time import sleep, strftime
from datetime import datetime

lcd = lcddriver.lcd()


lcd.lcd_display_string("emonPi", 1)
lcd.lcd_display_string("IP clock test",2)  
sleep(1)
 
cmd = "ip addr show wlan0 | grep inet | awk '{print $2}' | cut -d/ -f1 | head -n1"
#cmd = "ip addr show eth0 | grep inet | awk '{print $2}' | cut -d/ -f1 | head -n1"
 
 
def run_cmd(eth0):
        p = Popen(cmd, shell=True, stdout=PIPE)
        output = p.communicate()[0]
        return output

 
while 1:
        #lcd.lcd_clear()
        ipaddr = run_cmd(cmd)
        lcd.lcd_display_string(datetime.now().strftime('%b %d  %H:%M:%S\n'),1)
        lcd.lcd_display_string('IP %s' % ( ipaddr ),2)
        sleep(2)




