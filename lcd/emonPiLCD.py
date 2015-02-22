#!/usr/bin/env python

import lcddriver
lcd = lcddriver.lcd()

from subprocess import *
from time import sleep, strftime
from datetime import datetime

import threading

import RPi.GPIO as GPIO


# Use Pi board pin numbers as these as always consistent between revisions 
GPIO.setmode(GPIO.BOARD)                                 

#emonPi push button Pin 16 GPIO 23
GPIO.setup(16, GPIO.IN, pull_up_down = GPIO.PUD_DOWN)    




print datetime.now().strftime('%b %d %H:%M')

class ButtonInput():

    def __init__(self):
        GPIO.add_event_detect(16, GPIO.RISING, callback=self.buttonPress, bouncetime=500) 
        self.press_num = 0

    def buttonPress(self,channel):
        print('Button 1 pressed!') 
        self.press_num = self.press_num + 1 
        updatelcd()

buttoninput = ButtonInput()
#Setup callback function buttonpress to appen on press of push button    



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
    lcd.lcd_display_string( string_lenth(led_string1, 16),1) # line 1- make sure string is 16 characters long to fill LED 
    lcd.lcd_display_string( string_lenth(led_string2, 16),2) # line 2

def string_lenth(string, length):
	# Add blank characters to end of string to make up to length long
	if (len(string) < 16):
		string += ' ' * (16 - len(string))
	return (string)

 
while 1:

    if buttoninput.press_num == 0:   
        IP, network = local_IP()
        if IP == "":
            led_string1 = 'Awaiting Network'
            led_string2 = 'Connection......'
            
        if IP != "":
        	led_string1 = '%s connected' % (IP)
        	led_string2 = 'IP: %s' % (network)

    elif buttoninput.press_num == 1:          
        led_string1 = 'Checking WAN    '
        led_string2 = 'Connection......'
        #if is_connected() == True:
        #    led_string1 = 'Internet'
        #    led_string2 = 'Connected'
        #else:
        #    led_string1 = 'Internet'
        #    led_string2 = 'Connection FAIL'

    elif buttoninput.press_num == 2:     
        led_string1 = datetime.now().strftime('%b %d %H:%M')
        led_string2 = datetime.now().strftime('Uptime: 123 days')
    
    elif buttoninput.press_num == 3: 
    	led_string1 = 'Power: 563W'  
        led_string2 = 'Today: 1.3KW'      

    else:
        buttoninput.press_num = 0

#

            
GPIO.cleanup()
        




