#!/usr/bin/env python

import lcddriver 
lcd = lcddriver.lcd()
from subprocess import *
from time import sleep, strftime
from datetime import datetime
from datetime import timedelta
from uptime import uptime
import threading
import sys
import RPi.GPIO as GPIO


# Use Pi board pin numbers as these as always consistent between revisions 
GPIO.setmode(GPIO.BOARD)                                 

#emonPi LCD push button Pin 16 GPIO 23
GPIO.setup(16, GPIO.IN, pull_up_down = GPIO.PUD_DOWN)    
#emonPi Shutdown button
GPIO.setup(11, GPIO.IN)

print "OpenEnergyMonitor - emonPi LCD / Shutdown Python Script"

def shutdown():
    while (GPIO.input(11) == 1):
        lcd_string1 = "emonPi Shutdown"
        lcd_string2 = "5.."
        lcd.lcd_display_string( string_lenth(lcd_string1, 16),1)
        lcd.lcd_display_string( string_lenth(lcd_string2, 16),2)
        print lcd_string1
        sleep(1)
        for x in range(4, 0, -1):
            lcd_string2 += "%d.." % (x)
            lcd.lcd_display_string( string_lenth(lcd_string2, 16),2) 
            print lcd_string2
            sleep(1)
            
            if (GPIO.input(11) == 0):
                return
        lcd_string2="SHUTDOWN NOW!"
        lcd.lcd_display_string( string_lenth(lcd_string1, 16),1)
        lcd.lcd_display_string( string_lenth(lcd_string2, 16),2) 
        sleep(2)
        lcd.backlight(0)
        lcd.lcd_clear()
        lcd.lcd_display_string( string_lenth("Power", 16),1)
        lcd.lcd_display_string( string_lenth("Off", 16),2)
        sleep(2)
        call('halt', shell=False)
        sys.exit() #end script 


class ButtonInput():
    def __init__(self):
        GPIO.add_event_detect(16, GPIO.RISING, callback=self.buttonPress, bouncetime=200) 
        self.press_num = 0
    def buttonPress(self,channel):
        print self.press_num
        self.press_num = self.press_num + 1 
        #updatelcd()
buttoninput = ButtonInput()
#Setup callback function buttonpress to appen on press of push button    

def get_uptime():
    with open('/proc/uptime', 'r') as f:
        seconds = float(f.readline().split()[0])
        array = str(timedelta(seconds = seconds)).split('.')
        string = array[0]
    return string


# Return local IP address for eth0 or wlan0
def local_IP():
	wlan0 = "ip addr show wlan0 | grep inet | awk '{print $2}' | cut -d/ -f1 | head -n1"
	eth0 = "ip addr show eth0 | grep inet | awk '{print $2}' | cut -d/ -f1 | head -n1"
	p = Popen(eth0, shell=True, stdout=PIPE)
	IP = p.communicate()[0]
	IP = IP[:-1] 												#remove blank space at end of IP
	network = "eth0"
	if IP == "":
		p = Popen(wlan0, shell=True, stdout=PIPE)
		IP = p.communicate()[0].rstrip('\n')
		network = "wlan0"
	return {IP , network}
#IP, network = local_IP()
#print IP
#print network

#Test to see if we have working internet connection to emoncms.org 
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
#print 'Internet connected? %s' %(is_connected())

# write to I2C LCD 
def updatelcd():
    lcd.lcd_display_string( string_lenth(lcd_string1, 16),1) # line 1- make sure string is 16 characters long to fill LED 
    lcd.lcd_display_string( string_lenth(lcd_string2, 16),2) # line 2
#    print lcd_string1
#    print lcd_string2


def string_lenth(string, length):
	# Add blank characters to end of string to make up to length long
	if (len(string) < 16):
		string += ' ' * (16 - len(string))
	return (string)

def uptime_days():
    uptime_days = uptime() / 86400
    threading.Timer(60, uptime_days).start()     # every 60's update uptime display 
    return(uptime_days)


 
while 1:

    uptime_days()

    if buttoninput.press_num == 0:   
        IP, network = local_IP()
        if IP == "":
            lcd_string1 = 'Awaiting Network'
            lcd_string2 = 'Connection......'

        if IP != "":
        	lcd_string1 = '%s connected' % (network)
        	lcd_string2 = 'IP: %s' % (IP)

    #elif buttoninput.press_num == 1:          
    #    lcd_string1 = 'Checking WAN    '
    #    lcd_string2 = 'Connection......'
        #if is_connected() == True:
        #    lcd_string1 = 'Internet'
        #    lcd_string2 = 'Connected'
        #else:
        #    lcd_string1 = 'Internet'
        #    lcd_string2 = 'Connection FAIL'

    elif buttoninput.press_num == 1: 
 
        #print uptime('FORMAT_HOUR')
        lcd_string1 = datetime.now().strftime('%b %d %H:%M')
        lcd_string2 =  'Uptime %.2f days' % (uptime_days())
    
    elif buttoninput.press_num == 2: 
    	lcd_string1 = 'Power: 563W'  
        lcd_string2 = 'Today: 1.3KW'      

    else:
        buttoninput.press_num = 0

    if (GPIO.input(11) == 0): # only update LCD if shutdown button is not pressed
        updatelcd()
    else:
        shutdown()
        lcd.lcd_clear()




GPIO.cleanup()
        




