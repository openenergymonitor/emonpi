#!/usr/bin/env python
import lcddriver
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
import paho.mqtt.client as mqtt

# ------------------------------------------------------------------------------------
# LCD backlight timeout in seconds
# 0: always on
# 300: off after 5 min
# ------------------------------------------------------------------------------------
backlight_timeout = 300

# ------------------------------------------------------------------------------------
# emonPi Node ID (default 5)
# ------------------------------------------------------------------------------------
emonPi_nodeID = 5

# Default Startup Page
page = 0
max_number_pages = 3

# ------------------------------------------------------------------------------------
# Start Logging
# ------------------------------------------------------------------------------------
import logging
import logging.handlers
uselogfile = False

mqttc = False
mqttConnected = False
basedata = []

if not uselogfile:
    loghandler = logging.StreamHandler()
else:
    loghandler = logging.handlers.RotatingFileHandler("/var/log/emonPiLCD",'a', 5000 * 1024, 1)

loghandler.setFormatter(logging.Formatter('%(asctime)s %(levelname)s %(message)s'))
logger = logging.getLogger("emonPiLCD")
logger.addHandler(loghandler)    
logger.setLevel(logging.INFO)

logger.info("emonPiLCD Start")
# ------------------------------------------------------------------------------------

r = redis.Redis(host='localhost', port=6379, db=0)

# We wait here until redis has successfully started up
redisready = False
while not redisready:
    try:
        r.client_list()
        redisready = True
        
        
        
    except redis.ConnectionError:
        logger.info("waiting for redis-server to start...")
        time.sleep(1.0)

background = False

class Background(threading.Thread):

    def __init__(self):
        threading.Thread.__init__(self)
        self.stop = False
        
    def run(self):
        last1s = time.time() - 2.0
        last5s = time.time() - 6.0
        logger.info("Starting background thread")
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

                # Ethernet
                # --------------------------------------------------------------------------------
                eth0 = "ip addr show eth0 | grep inet | awk '{print $2}' | cut -d/ -f1 | head -n1"
                p = Popen(eth0, shell=True, stdout=PIPE)
                eth0ip = p.communicate()[0][:-1]
                
                ethactive = 1
                if eth0ip=="" or eth0ip==False:
                    ethactive = 0
                    
                r.set("eth:active",ethactive)
                r.set("eth:ip",eth0ip)
                logger.info("background: eth:"+str(int(ethactive))+" "+eth0ip)
                
                # Wireless LAN
                # ----------------------------------------------------------------------------------
                wlan0 = "ip addr show wlan0 | grep inet | awk '{print $2}' | cut -d/ -f1 | head -n1"
                p = Popen(wlan0, shell=True, stdout=PIPE)
                wlan0ip = p.communicate()[0][:-1]
                
                wlanactive = 1
                if wlan0ip=="" or wlan0ip==False:
                    wlanactive = 0
                    
                r.set("wlan:active",wlanactive)
                r.set("wlan:ip",wlan0ip)
                logger.info("background: wlan:"+str(int(wlanactive))+" "+wlan0ip)
                
                # ----------------------------------------------------------------------------------
        
                signallevel = 0
                linklevel = 0
                noiselevel = 0        

                if wlanactive:
                    # wlan link status
                    p = Popen("/sbin/iwconfig wlan0", shell=True, stdout=PIPE)
                    iwconfig = p.communicate()[0]
                    tmp = re.findall('(?<=Signal level=)\w+',iwconfig)
                    if len(tmp)>0: signallevel = tmp[0]

                r.set("wlan:signallevel",signallevel)
                logger.info("background: wlan "+str(signallevel))
                
            # this loop runs a bit faster so that ctrl-c exits are fast
            time.sleep(0.1)
            
def sigint_handler(signal, frame):
    lcd_string1 = "LCD SCRIPT"
    lcd_string2 =  "STOPPED"
    lcd.lcd_display_string( string_lenth(lcd_string1, 16),1)
    lcd.lcd_display_string( string_lenth(lcd_string2, 16),2) 
    time.sleep(1)
    logger.info("ctrl+c exit received")
    background.stop = True;
    sys.exit(0)
    
def sigterm_handler(signal, frame):
    lcd_string1 = "LCD SCRIPT"
    lcd_string2 =  "STOPPED"
    lcd.lcd_display_string( string_lenth(lcd_string1, 16),1)
    lcd.lcd_display_string( string_lenth(lcd_string2, 16),2) 
    time.sleep(1)
    logger.info("sigterm received")
    background.stop = True;
    sys.exit(0)
    
def shutdown():
    while (GPIO.input(11) == 1):
        lcd_string1 = "emonPi Shutdown"
        lcd_string2 = "5.."
        lcd.lcd_display_string( string_lenth(lcd_string1, 16),1)
        lcd.lcd_display_string( string_lenth(lcd_string2, 16),2)
        logger.info("main lcd_string1: "+lcd_string1)
        time.sleep(1)
        for x in range(4, 0, -1):
            lcd_string2 += "%d.." % (x)
            lcd.lcd_display_string( string_lenth(lcd_string2, 16),2) 
            logger.info("main lcd_string2: "+lcd_string2)
            time.sleep(1)
            
            if (GPIO.input(11) == 0):
                return
        lcd_string2="SHUTDOWN NOW!"
        background.stop = True
        lcd.lcd_display_string( string_lenth(lcd_string1, 16),1)
        lcd.lcd_display_string( string_lenth(lcd_string2, 16),2) 
        time.sleep(2)
        lcd.lcd_clear()
        lcd.lcd_display_string( string_lenth("Wait 30s...", 16),1)
        lcd.lcd_display_string( string_lenth("Before Unplug!", 16),2)
        time.sleep(4)
        lcd.backlight(0) 											# backlight zero must be the last call to the LCD to keep the backlight off 
        call('halt', shell=False)
        sys.exit() #end script 
                    
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
    lcd.lcd_display_string( string_lenth(lcd_string2, 16),2) # line 2
    
def on_connect(client, userdata, flags, rc):
    global mqttConnected
    if rc:
        mqttConnected = False
    else:
        logger.info("MQTT Connection UP")
        mqttConnected = True
        mqttc.subscribe("emonhub/rx/#")
    
def on_disconnect(client, userdata, rc):
    global mqttConnected
    logger.info("MQTT Connection DOWN")
    mqttConnected = False

def on_message(client, userdata, msg):
    topic_parts = msg.topic.split("/")
    logger.info("MQTT RX: "+msg.topic+" "+msg.payload)
    if int(topic_parts[2])==emonPi_nodeID:
        basedata = msg.payload.split(",")
        r.set("basedata",msg.payload)

class ButtonInput():
    def __init__(self):
        GPIO.add_event_detect(16, GPIO.RISING, callback=self.buttonPress, bouncetime=1000) 
        self.press_num = 0
        self.pressed = False
    def buttonPress(self,channel):
        self.pressed = True
        logger.info("lcd button press "+str(self.press_num))
        
signal.signal(signal.SIGINT, sigint_handler)
signal.signal(signal.SIGTERM,sigterm_handler)

# Use Pi board pin numbers as these as always consistent between revisions 
GPIO.setmode(GPIO.BOARD)                                 
#emonPi LCD push button Pin 16 GPIO 23
GPIO.setup(16, GPIO.IN, pull_up_down = GPIO.PUD_DOWN)    
#emonPi Shutdown button, Pin 11 GPIO 17
GPIO.setup(11, GPIO.IN)

time.sleep(1.0)

lcd_string1 = ""
lcd_string2 = ""

background = Background()
background.start()
buttoninput = ButtonInput()

lcd = lcddriver.lcd()

mqttc = mqtt.Client()
mqttc.on_connect = on_connect
mqttc.on_disconnect = on_disconnect
mqttc.on_message = on_message

last1s = time.time() - 1.0
buttonPress_time = time.time()

while 1:

    now = time.time()

    if not mqttConnected:
        logger.info("Connecting to MQTT Server")
        try:
            mqttc.connect("127.0.0.1", "1883", 60)
            lcd = lcddriver.lcd()
        except:
            logger.info("Could not connect...")
            time.sleep(1.0)
    
    mqttc.loop(0)

    if buttoninput.pressed:
        if backlight == True: page = page + 1
        if page>max_number_pages: page = 0
        buttonPress_time = time.time()

        #turn backight off afer x seconds 
    if (now - buttonPress_time) > backlight_timeout: 
        backlight = False
        lcd.backlight(0) 
        if GPIO.input(11) == 1: shutdown() #ensure shutdown button works when backlight is off
    else: backlight = True
    
    
    # ----------------------------------------------------------
    # UPDATE EVERY 1's
    # ----------------------------------------------------------
    if ((now-last1s)>=1.0 and backlight) or buttoninput.pressed:
        last1s = now

        if page==0:  
            if int(r.get("eth:active")):
                lcd_string1 = "Ethernet: YES"
                lcd_string2 = r.get("eth:ip")
            else:
            	if int(r.get("wlan:active")): 
            		page=page+1
            	else:
            		lcd_string1 = "Ethernet:"
            		lcd_string2 = "NOT CONNECTED"
                
        elif page==1:
            if int(r.get("wlan:active")):
                lcd_string1 = "WIFI: YES  "+str(r.get("wlan:signallevel"))+"%"
                lcd_string2 = r.get("wlan:ip")
            else:
                lcd_string1 = "WIFI:"
                lcd_string2 = "NOT CONNECTED"
                
        elif page==2:
		    lcd_string1 = datetime.now().strftime('%b %d %H:%M')
		    lcd_string2 =  'Uptime %.2f days' % (float(r.get("uptime"))/86400)
		    
        elif page==3: 
            basedata = r.get("basedata")
            if basedata is not None:
                basedata = basedata.split(",")
                lcd_string1 = 'Power 1: '+str(basedata[0])+"W"
                lcd_string2 = 'Power 2: '+str(basedata[1])+"W"
            else:
                lcd_string1 = 'Power 1: ...'
                lcd_string2 = 'Power 2: ...'
        
        logger.info("main lcd_string1: "+lcd_string1)
        logger.info("main lcd_string2: "+lcd_string2)
        
        # If Shutdown button is not pressed update LCD
        if (GPIO.input(11) == 0):
            updatelcd()
        # If Shutdown button is pressed initiate shutdown sequence 
        else:
            logger.info("shutdown button pressed")
            shutdown()

    buttoninput.pressed = False
    time.sleep(0.1)
    
GPIO.cleanup()
logging.shutdown()
