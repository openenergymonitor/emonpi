#!/usr/bin/env python

import lcddriver 
lcd = lcddriver.lcd()
from subprocess import *
import time
from datetime import datetime
from datetime import timedelta
from uptime import uptime
import threading
import sys
import RPi.GPIO as GPIO
import signal
import redis
import re

r = redis.Redis(host='localhost', port=6379, db=0)

# We wait here until redis has successfully started up
redisready = False
while not redisready:
    try:
        r.client_list()
        redisready = True
    except redis.ConnectionError:
	print "waiting for redis-server to start..."
        time.sleep(1.0)

background = False

class Background(threading.Thread):

    def __init__(self):
        threading.Thread.__init__(self)
        self.stop = False
        
    def run(self):
        last1s = time.time() - 2.0
        last5s = time.time() - 6.0
        # Loop until we stop is false (our exit signal)
        while not self.stop:
            now = time.time()
            
            # ----------------------------------------------------------
            # UPDATE EVERY 1's
            # ----------------------------------------------------------
            if (now-last1s)>=1.0:
                last1s = now
                # Get uptime
                with open('/proc/uptime', 'r') as f:
                    seconds = float(f.readline().split()[0])
                    array = str(timedelta(seconds = seconds)).split('.')
                    string = array[0]
                    r.set("uptime",seconds)
                    
            # ----------------------------------------------------------
            # UPDATE EVERY 5's
            # ----------------------------------------------------------
            if (now-last5s)>=5.0:
                last5s = now
                
                # Ip address's
                wlan0 = "ip addr show wlan0 | grep inet | awk '{print $2}' | cut -d/ -f1 | head -n1"
                eth0 = "ip addr show eth0 | grep inet | awk '{print $2}' | cut -d/ -f1 | head -n1"
                p = Popen(eth0, shell=True, stdout=PIPE)
                IP = p.communicate()[0]
                IP = IP[:-1] #remove blank space at end of IP
                network = "eth"
                if IP == "":
                    p = Popen(wlan0, shell=True, stdout=PIPE)
                    IP = p.communicate()[0].rstrip('\n')
                    network = "wlan"
                r.set("ipaddress",IP)
                r.set("network",network)
                
                if network=="wlan":
                    # wlan link status
                    p = Popen("/sbin/iwconfig wlan0", shell=True, stdout=PIPE)
                    iwconfig = p.communicate()[0]
                    signallevel = re.findall('(?<=Signal level=)\w+',iwconfig)[0]
                    linklevel = re.findall('(?<=Link Quality=)\w+',iwconfig)[0]
                    noiselevel = re.findall('(?<=Noise level=)\w+',iwconfig)[0]
                    r.set("wlan:signallevel",signallevel)
                    r.set("wlan:linklevel",linklevel)
                    r.set("wlan:noiselevel",noiselevel)
                
            # this loop runs a bit faster so that ctrl-c exits are fast
            time.sleep(0.1)
            
def sigint_handler(signal, frame):
    """Catch SIGINT (Ctrl+C)."""
    print "SIGINT received."
    background.stop = True;
    sys.exit(0)

def shutdown():
    while (GPIO.input(11) == 1):
        lcd_string1 = "emonPi Shutdown"
        lcd_string2 = "5.."
        lcd.lcd_display_string( string_lenth(lcd_string1, 16),1)
        lcd.lcd_display_string( string_lenth(lcd_string2, 16),2)
        print lcd_string1
        time.sleep(1)
        for x in range(4, 0, -1):
            lcd_string2 += "%d.." % (x)
            lcd.lcd_display_string( string_lenth(lcd_string2, 16),2) 
            print lcd_string2
            time.sleep(1)
            
            if (GPIO.input(11) == 0):
                return
        lcd_string2="SHUTDOWN NOW!"
        lcd.lcd_display_string( string_lenth(lcd_string1, 16),1)
        lcd.lcd_display_string( string_lenth(lcd_string2, 16),2) 
        time.sleep(2)
        lcd.lcd_clear()
        lcd.lcd_display_string( string_lenth("Power", 16),1)
        lcd.lcd_display_string( string_lenth("Off", 16),2)
        lcd.backlight(0) 											# backlight zero must be the last call to the LCD to keep the backlight off 
        call('halt', shell=False)
        sys.exit() #end script 

class ButtonInput():
    def __init__(self):
        GPIO.add_event_detect(16, GPIO.RISING, callback=self.buttonPress, bouncetime=1000) 
        self.press_num = 0
        self.pressed = False
    def buttonPress(self,channel):
        print self.press_num
        self.press_num = self.press_num + 1
        if self.press_num>3: self.press_num = 0
        self.pressed = True
                    
def get_uptime():

    return string

def string_lenth(string, length):
	# Add blank characters to end of string to make up to length long
	if (len(string) < 16):
		string += ' ' * (16 - len(string))
	return (string)

# write to I2C LCD 
def updatelcd():
    # line 1- make sure string is 16 characters long to fill LED 
    lcd.lcd_display_string( string_lenth(lcd_string1, 16),1)
    print lcd_string1
    lcd.lcd_display_string( string_lenth(lcd_string2, 16),2) # line 2
    print lcd_string2

# Use Pi board pin numbers as these as always consistent between revisions 
GPIO.setmode(GPIO.BOARD)                                 
#emonPi LCD push button Pin 16 GPIO 23
GPIO.setup(16, GPIO.IN, pull_up_down = GPIO.PUD_DOWN)    
#emonPi Shutdown button, Pin 11 GPIO 17
GPIO.setup(11, GPIO.IN)

print "OpenEnergyMonitor - emonPi LCD / Shutdown Python Script"

lcd_string1 = ""
lcd_string2 = ""

signal.signal(signal.SIGINT, sigint_handler)

background = Background()
background.start()
buttoninput = ButtonInput()

last1s = time.time() - 1.0
while 1:
    now = time.time()
    
    # ----------------------------------------------------------
    # UPDATE EVERY 1's
    # ----------------------------------------------------------
    if (now-last1s)>=1.0 or buttoninput.pressed:
        last1s = now
        buttoninput.pressed = False
        
        if buttoninput.press_num==0:
            IP = r.get("ipaddress")
            if IP == "" or IP == None:
                lcd_string1 = 'Awaiting Network'
                lcd_string2 = 'Connection......'
            else:
                lcd_string1 = str(r.get("network"))
                signallevel = r.get("wlan:signallevel")
                if signallevel is not None:
                    lcd_string1 += "    SIG:"+str()+"%"
                lcd_string2 = "IP "+str(IP)
            
        elif buttoninput.press_num==1:
            lcd_string1 = "Link|Sig|Noise"
            if r.get("network")=="wlan":
                lcd_string2 = r.get("wlan:linklevel")+"% "+r.get("wlan:signallevel")+"% "+r.get("wlan:noiselevel")+"%"
            else:
                lcd_string2 = ""

        elif buttoninput.press_num==2:
		    lcd_string1 = datetime.now().strftime('%b %d %H:%M')
		    lcd_string2 =  'Uptime %.2f days' % (float(r.get("uptime"))/86400)
		    
        elif buttoninput.press_num==3: 
            lcd_string1 = 'Power: 563W'
            lcd_string2 = 'Today: 1.3KW'
        
        if (GPIO.input(11) == 0):
            updatelcd()
        else:
            shutdown()
    
    time.sleep(0.1)
    
GPIO.cleanup()
        




