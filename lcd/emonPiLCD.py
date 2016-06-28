#!/usr/bin/env python
import lcddriver
import subprocess
from subprocess import *
import time
from datetime import datetime
from datetime import timedelta
#from gprs_signal import previous_signalf
from  gprs_signal import get_gsm_signal_strength, switch_off_gsm
from uptime import uptime
import threading
import sys
import Adafruit_BBIO.GPIO as GPIO
import signal
import redis
import re
import paho.mqtt.client as mqtt


# ------------------------------------------------------------------------------------
# LCD backlight timeout in seconds
# 0: always on
# 300: off after 5 min
# ------------------------------------------------------------------------------------
backlight_timeout = 180

# ------------------------------------------------------------------------------------
# emonPi Node ID (default 5)
# ------------------------------------------------------------------------------------
emonPi_nodeID = 10
lcd = lcddriver.lcd()


# Default Startup Page
page = 0
inc = 0
GPIO_PORT = "P8_11"

#in case we use a button to switch on/off
GPIO_PORT_shutdown = "P8_12"
GPIO.setup( GPIO_PORT,GPIO.IN)
GPIO.setup( GPIO_PORT_shutdown,GPIO.IN)
new_switch_state = GPIO.input(GPIO_PORT)
shutdown_button =GPIO.input(GPIO_PORT_shutdown)

max_number_pages = 6

SHUTDOWN_TIME =3  # Shutdown after 3 second press

# ------------------------------------------------------------------------------------
# Start Logging
# ------------------------------------------------------------------------------------
import logging
import logging.handlers
# NOTE this is during pilots remove this in future or move to proper log management
uselogfile = True

mqttc = False
mqttConnected = False
basedata = []
mqtt_rx_channel = "emonhub/rx/#"
mqtt_push_channel = "emonhub/tx/#"

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
        last30s = time.time() - 31.0
        last100s = time.time() - 100.0   #update the GPRS signal strength after 5min
        last5h = time.time() - 18000   #update the GPRS signal strength after 5min
        logger.info("Starting background thread")
        # Loop until we stop is false (our exit signal)

        pppactive = 0
        wlanactive = 0
        ethactive = 0
        data_counter_init = False
        while not self.stop:
            try:

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

	   # GPRS data sent counter
	   # ----------------------------------------------------------------------------------
                        if pppactive:

			   ppp0 = "ifconfig ppp0 | grep -oP '(?<=TX bytes:)[0-9]*'"
			   p = Popen(ppp0, shell=True, stdout=PIPE)
			   ppp0tx = p.communicate()[0]

                           oldtx = r.get("ppp:old_tx")
                           oldrx = r.get("ppp:old_rx")

			   ppp0 = "ifconfig ppp0 | grep -oP '(?<=RX bytes:)[0-9]*'"
			   p = Popen(ppp0, shell=True, stdout=PIPE)
			   ppp0rx = p.communicate()[0]

                           if oldtx and oldrx:
                              oldtx = int(oldtx)
                              oldrx = int(oldrx)
                              ppp0tx = oldtx + int(ppp0tx)
                              ppp0rx = oldrx + int(ppp0rx)


			   r.set("ppp:tx",ppp0tx)
			   r.set("ppp:rx",ppp0rx)
			   logger.info("background: ppp data tx:"+str(ppp0tx))
			   logger.info("background: ppp data rx:"+str(ppp0rx))
                           data_counter_init = True
		    if (now - last100s) >= 100.0:
			last100s = now

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

	# GPRS connection from olemexino GSM module
	# ----------------------------------------------------------------------------------
			wlanactive = 0 # Not using wlan
			ppp0 = "ip addr show ppp0 | grep inet | awk '{print $2}' | cut -d/ -f1 | head -n1"
			p = Popen(ppp0, shell=True, stdout=PIPE)
			ppp0ip = p.communicate()[0][:-1]


			pppactive = 1
			if ppp0ip=="" or ppp0ip==False:
			    pppactive = 0
			    time.sleep(1)
			    #subprocess.call(['/home/debian/gprsAndEmonInstall/gprs_on.sh'])


			r.set("ppp:active",pppactive)
			r.set("ppp:ip",ppp0ip)
			logger.info("background: ppp:"+str(int(pppactive))+" "+ppp0ip)
	  #-------------------------------------------------------------------------------------

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

			gsm_signallevel = 0

		    if (now - last5h) >= 18000.0:
			last5h = now
			if pppactive:
   	   #---------------------------gprs signal level----------------------------------------
                           if data_counter_init:
				   r.set("ppp:old_tx",ppp0tx)
				   r.set("ppp:old_rx",ppp0rx)
			   gsm_signallevel = get_gsm_signal_strength()
			   #print "$#$#$#$#$#$$# %s"%gsm_signallevel
			   r.set("ppp:gsm_signallevel",gsm_signallevel)
			   logger.info("background: ppp "+str(gsm_signallevel))

            except Exception,e:
               logger.exception("An error occured in thread")

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
    while (shutdown_button == 1):
        lcd_string1 = "RMC REBOOT"
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

            if (shutdown_button == 0):
                return
        lcd_string2="SHUTTING DOWN..."
        background.stop = True
        lcd.lcd_display_string( string_lenth(lcd_string1, 16),1)
        lcd.lcd_display_string( string_lenth(lcd_string2, 16),2)
        time.sleep(2)
        lcd.lcd_clear()
        lcd.lcd_display_string( string_lenth("Rebooting...", 16),1)
        lcd.lcd_display_string( string_lenth("Please wait", 16),2)
        time.sleep(4)
	switch_off_gsm()
#from gprs_signal import previous_signalf
        time.sleep(1)

        lcd.backlight(0) 	# backlight zero must be the last call to the LCD to keep the backlight off
        call('reboot', shell=False)	#
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
        mqttc.subscribe(mqtt_rx_channel)

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
        #GPIO.add_event_detect(GPIO_PORT, GPIO.RISING, callback=self.buttonPress, bouncetime=1000)
        self.press_num = 0
        self.pressed = False
    def buttonPress(self,channel):

        self.pressed = True
        logger.info("lcd button press "+str(self.press_num))

signal.signal(signal.SIGINT, sigint_handler)
signal.signal(signal.SIGTERM,sigterm_handler)

# Use Pi board pin numbers as these as always consistent between revisions
#GPIO.setmode(GPIO.BOARD)
#emonPi LCD push button Pin 16 GPIO 23
#GPIO.setup(16, GPIO.IN, pull_up_down = GPIO.PUD_DOWN)
#emonPi Shutdown button, Pin 11 GPIO 17
#GPIO.setup(11, GPIO.IN)

time.sleep(1.0)

lcd_string1 = ""
lcd_string2 = ""

background = Background()
background.start()
buttoninput = ButtonInput()

mqttc = mqtt.Client()
mqttc.on_connect = on_connect
mqttc.on_disconnect = on_disconnect
mqttc.on_message = on_message

last1s = time.time() - 1.0
buttonPress_time = time.time()

while 1:

    logging.info("Starting main loop")

    if background.is_alive():
       logging.info("thread is still alive")
    else:
       logging,info("thread is dead - needs restart")


    #while GPIO.input(GPIO_PORT) == GPIO.LOW:
    #   time.sleep(0.1)
    #   logging.info("button not pressed")

    # Get the time button was pressed
    button_down_time = time.time()
    buttoninput.pressed = False

    while GPIO.input(GPIO_PORT) == GPIO.HIGH:
       backlight = True
       time.sleep(0.1)
       buttoninput.pressed = True

       logging.info("Button Pressed")
       if time.time() - button_down_time > SHUTDOWN_TIME:
          logging.info("Button Pressed looong" )
          shutdown_button = 1
	  break
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
        if page > max_number_pages: page = 0
        buttonPress_time = time.time()

        #turn backight off afer x seconds
    if (now - buttonPress_time) > backlight_timeout:
        backlight = False
        lcd.backlight(0)
        if shutdown_button == 1: shutdown() #ensure shutdown button works when backlight is off
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
                # AT - Removing int casting. not necassary
                if r.get("wlan:active"):
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
                if int(r.get("ppp:active")):
			lcd_string1 = "GSM: YES - "+r.get("ppp:gsm_signallevel")+"%"
			lcd_string2 = r.get("ppp:ip")
               #print  "SIGNAL STRENGTH" + lcd_string1
                #print"********************CONNECTED!!!!!!!!"+r.get("ppp:ip")

		else:
			lcd_string1 = "GSM:"
			lcd_string2 = "NOT CONNECTED"
                #print  "SIGNAL STRENGTH" + lcd_string1
                #print"****************NOT CONNECTED"


        elif page==3:
		    lcd_string1 = datetime.now().strftime('%b %d %H:%M')
		    lcd_string2 =  'Uptime %.2f days' % (float(r.get("uptime"))/86400)

        elif page==4:
            basedata = r.get("basedata")
            if basedata is not None:
                basedata = basedata.split(",")
                lcd_string1 = 'Power 1: '+str(basedata[0])+"W"
                lcd_string2 = 'Power 2: '+str(basedata[1])+"W"
            else:
                lcd_string1 = 'Power 1: ...'
                lcd_string2 = 'Power 2: ...'

        elif page==5:
            basedata = r.get("basedata")
            if basedata is not None:
                basedata = basedata.split(",")
                lcd_string1 = 'Power 3: '+str(basedata[2])+"W"
                lcd_string2 = 'Power 4: '+str(basedata[3])+"W"
             #   print"***********************power 3:"
             #   print"**************************power 4:"

            else:
                lcd_string1 = 'Power 3: ...'
                lcd_string2 = 'Power 4: ...'
             #   print"*******************power 3:....."
             #   print"********************power 4:......"

        elif page==6:
            tx = r.get("ppp:tx")
            rx = r.get("ppp:rx")

            if tx and rx is not None:
		tx = int (tx)/1024
		rx = int (rx)/1024
                lcd_string1 = 'Data TX : '+str(tx)+"kb"
                lcd_string2 = 'Data RX. : '+str(rx)+"kb"
             #   print"***********************power 3:"
             #   print"**************************power 4:"

            else:
                lcd_string1 = 'Data Sent : ...'
                lcd_string2 = 'Data Rec. : ...'
             #   print"*******************power 3:....."
             #   print"********************power 4:......"



        logger.info("main lcd_string1: "+lcd_string1)
        logger.info("main lcd_string2: "+lcd_string2)

        # If Shutdown button is not pressed update LCD
        if (shutdown_button == 0):
            updatelcd()
        # If Shutdown button is pressed initiate shutdown sequence
        else:
            logger.info("shutdown button pressed")
            shutdown()

    buttoninput.pressed = False
    time.sleep(0.1)

GPIO.cleanup()
logging.shutdown()

