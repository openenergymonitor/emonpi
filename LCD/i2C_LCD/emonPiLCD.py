#!/usr/bin/env python
import lcddriver
from subprocess import *
from time import sleep, strftime
from datetime import datetime
lcd = lcddriver.lcd()


software_rev= 0.1
firmware_rev=0.1

print 'emonPi V%s' %(software_rev)
print 'Firmware: V%s'%(firmware_rev)
print datetime.now().strftime('%b %d %H:%M')

def lcd_startup(software_rev, firmware_rev):
    lcd.lcd_display_string('emonPi V%s' %(software_rev), 1)
    lcd.lcd_display_string('Firmware: V%s'%(firmware_rev) ,2)  
    sleep(2)
    return()
lcd_startup(software_rev, firmware_rev)
 
# Return local IP address for eth0 or wlan0
def local_IP():
	wlan0 = "ip addr show wlan0 | grep inet | awk '{print $2}' | cut -d/ -f1 | head -n1"
	eth0 = "ip addr show eth0 | grep inet | awk '{print $2}' | cut -d/ -f1 | head -n1"
	p = Popen(eth0, shell=True, stdout=PIPE)
	IP = p.communicate()[0]
	network = "eth0"
	if IP == "":
		p = Popen(wlan0, shell=True, stdout=PIPE)
		IP = p.communicate()[0].rstrip('\n')
		network = "wlan0"
	return {IP , network}
IP, network = local_IP()
print IP
print network

#Test to see if we have working internet connection 
import socket
REMOTE_SERVER = "www.google.com"
def is_connected():
  try:
    # see if we can resolve the host name -- tells us if there is
    # a DNS listening
    host = socket.gethostbyname(REMOTE_SERVER)
    # connect to the host -- tells us if the host is actually
    # reachable
    s = socket.create_connection((host, 80), 2)
    return True
  except:
     pass
  return False
print 'Internet connected? %s' %(is_connected())

sleep(5)
 
while 1:
        lcd.lcd_clear()
        IP, network = local_IP()

        if IP == "":
            lcd.lcd_display_string('Awaiting Network',1) 
            lcd.lcd_display_string('Connection......',2)
            
        if IP != "":
            lcd.lcd_display_string('%s connected' % (network),1)
            lcd.lcd_display_string('IP: %s' % ( IP ),2)

        sleep(5)
        lcd.lcd_clear()

        if is_connected() == True:
        	lcd.lcd_display_string('Internet',1)
        	lcd.lcd_display_string('Connected',2)
        else:
            lcd.lcd_display_string('Internet',1)
            lcd.lcd_display_string('Connection FAIL',2)

        sleep(5)

        lcd.lcd_clear()
        lcd.lcd_display_string(datetime.now().strftime('%b %d %H:%M'),1)
        lcd.lcd_display_string(datetime.now().strftime('Uptime: 123 days'),2)
        sleep(5)

       
        lcd.lcd_clear()
        lcd.lcd_display_string(datetime.now().strftime('Found AC Voltage'),1)
        lcd.lcd_display_string(datetime.now().strftime('240.2V RMS'),2)
        sleep(5)

        lcd.lcd_clear()
        lcd.lcd_display_string(datetime.now().strftime('Real Power '),1)
        lcd.lcd_display_string(datetime.now().strftime('Sampling Active'),2)
        sleep(5)
  
        lcd.lcd_clear()
        lcd.lcd_display_string(datetime.now().strftime('CT1 Now: 25000W'),1)
        lcd.lcd_display_string(datetime.now().strftime('CT2 Now: 120W'),2)
        sleep(5)

        lcd.lcd_clear()
        lcd.lcd_display_string(datetime.now().strftime('CT1:005451.2 KWh'),1)
        lcd.lcd_display_string(datetime.now().strftime('CT2:000065.2 KWh'),2)
        sleep(5)

        lcd.lcd_clear()
        lcd.lcd_display_string(datetime.now().strftime('Found 3 DS18B20'),1)
        lcd.lcd_display_string(datetime.now().strftime('20.2 18.3 16.5'),2)
        sleep(2)
        lcd.lcd_display_string(datetime.now().strftime('Temperatures:  '),1)
        sleep(5)

        




