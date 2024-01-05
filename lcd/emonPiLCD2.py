#!/usr/bin/python3

# pylint: disable=line-too-long

import time
from datetime import datetime
import subprocess
import sys
import socket
import fcntl
import struct
import logging
import logging.handlers
import os
import configparser
import itertools
import threading
import math

import redis
import paho.mqtt.client as mqtt
from gpiozero import Button

# Local files

import board
from PIL import Image, ImageDraw, ImageFont
import adafruit_ssd1306

import gsmhuaweistatus

path = os.path.dirname(os.path.realpath(__file__))
# ------------------------------------------------------------------------------------
# Script version
version = '5'
# ------------------------------------------------------------------------------------

config = configparser.ConfigParser()
config.read(path + '/emonPiLCD.cfg')  # FIXME should live in /etc, not /usr/share/emonPiLCD

# ------------------------------------------------------------------------------------
# MQTT Settings
# ------------------------------------------------------------------------------------
mqtt_user = config.get('mqtt', 'mqtt_user')
mqtt_passwd = config.get('mqtt', 'mqtt_passwd')
mqtt_host = config.get('mqtt', 'mqtt_host')
mqtt_port = config.getint('mqtt', 'mqtt_port')
mqtt_topics = {config.get('mqtt', 'mqtt_temp1_topic'): 'temp1',
               config.get('mqtt', 'mqtt_temp2_topic'): 'temp2',
               config.get('mqtt', 'mqtt_feed1_topic'): 'feed1',
               config.get('mqtt', 'mqtt_feed2_topic'): 'feed2',
               config.get('mqtt', 'mqtt_vrms_topic'): 'vrms',
               config.get('mqtt', 'mqtt_pulse_topic'): 'pulse',
              }

# ------------------------------------------------------------------------------------
# Redis Settings
# ------------------------------------------------------------------------------------
redis_host = config.get('redis', 'redis_host')
redis_port = config.get('redis', 'redis_port')
r = redis.Redis(host=redis_host, port=redis_port, db=0, charset="utf-8", decode_responses=True)

# ------------------------------------------------------------------------------------
# General Settings
# ------------------------------------------------------------------------------------

# LCD backlight timeout in seconds 0: always on, 300: off after 5 min
backlight_timeout = config.getint('general', 'backlight_timeout')
default_page = config.getint('general', 'default_page')

# Threadlocker used to lock the thread during LCD updates 
lock = threading.Lock()

#Names to be displayed on power reading page
feed1_name = config.get('general', 'feed1_name')
feed2_name = config.get('general', 'feed2_name')
feed1_unit = config.get('general', 'feed1_unit')
feed2_unit = config.get('general', 'feed2_unit')

#How often the LCD is updated when a button is not pressed
lcd_update_sec = config.getint('general', 'lcd_update_sec')
# ------------------------------------------------------------------------------------
# Huawei Hi-Link GSM/3G USB dongle IP address on eth1
# ------------------------------------------------------------------------------------
hilink_device_ip = config.get('huawei', 'hilink_device_ip')

# ------------------------------------------------------------------------------------
# I2C LCD: each I2C address will be tried in consecutive order until LCD is found
# The first address that matches a device on the I2C bus will be used for the I2C LCD
# ------------------------------------------------------------------------------------
lcd_i2c = ['3c']
# ------------------------------------------------------------------------------------

# Default Startup Page
max_number_pages = 8
page = default_page
screensaver = False

sd_image_version = ''

sshConfirm = False
shutConfirm = False
wifiAP_confirm = False

# ------------------------------------------------------------------------------------
# Start Logging
# ------------------------------------------------------------------------------------
uselogfile = config.get('general', 'uselogfile')
logger = logging.getLogger("emonPiLCD")

#ssh enable/disable/check commands
ssh_enable = "sudo systemctl enable ssh > /dev/null"
ssh_start = "sudo systemctl start ssh > /dev/null"
ssh_disable = "sudo systemctl disable ssh > /dev/null"
ssh_stop = "sudo systemctl stop ssh > /dev/null"
ssh_status = "sudo systemctl status ssh > /dev/null"

wifiAP_start = "sudo systemctl enable --now wpa_supplicant@ap0.service > /dev/null"
wifiAP_stop = "sudo systemctl disable --now wpa_supplicant@ap0.service > /dev/null"
wifiAP_status = "ifconfig | grep 'inet 192.168.42.1'"

oled_last = True

def drawClear():
    global draw
    global oled
    global image

    draw.rectangle((0, 0, 128, 32), outline=0, fill=0)
    oled.image(image)
    oled.show()
        
def drawText(x,y,msg,update=False):
    global draw
    global oled
    global font
    global image
    global oled_last
    
    if oled_last:
        draw.rectangle((0, 0, 128, 32), outline=0, fill=0)
        oled_last = False
    
    draw.text((x,y), msg, font=font, fill=255)
    
    if update:
        oled.image(image)
        oled.show()
        oled_last = True


def buttonPressLong():
    global page
    
    logger.info("Mode button LONG press")

    if page == 7:
        ret = subprocess.call(ssh_status, shell=True)
        if ret > 0:
            drawText(0,0,'Enabling SSH',True)
            #ssh not running, enable & start it
            subprocess.call(ssh_enable, shell=True)
            subprocess.call(ssh_start, shell=True)
            logger.info("SSH Enabled")
            drawText(0,0,'SSH Enabled')
            drawText(0,14,'Change password!',True)
        else:
            drawText(0,0,'Disabling SSH',True)
            #disable ssh
            subprocess.call(ssh_disable, shell=True)
            subprocess.call(ssh_stop, shell=True)
            logger.info("SSH Disabled")
            drawText(0,0,'SSH Disabled')
            drawText(0,14,'',True)

    elif page == 8:
        logger.info("Shutting down")
        shutdown()
        
    elif page == 5:
        ret = subprocess.call(wifiAP_status, shell=True)       
        if ret == 0:
            logger.info("Stopping WiFi AP")
            drawText(0,0,'Stopping WiFi AP',True)
            subprocess.call(wifiAP_stop, shell=True)
            time.sleep(1)
            drawText(0,0,'WiFi AP Stopped',True)
            time.sleep(1)
            page = 4
            updateLCD()
        else:
            logger.info("Starting WiFi AP")
            drawText(0,0,'Starting WiFi AP',True)
            subprocess.call(wifiAP_start, shell=True)
            time.sleep(3)
            drawText(0,0,'WiFi AP Started',True)
            time.sleep(1)
            page = 4
            updateLCD()

def buttonPress():
    global page
    global screensaver
    global buttonPress_time
    
    if screensaver:
        screensaver = False
    else:
        page += 1
        if page > max_number_pages:
            page = 0

    now = time.time()
    buttonPress_time = now
    
    logger.info("Mode button SHORT press")
    logger.info("Page: " + str(page))
    updateLCD()


def updateLCD():
    # Lock thread to avoid LCD being corrupted if a button press interrupt interrupts the LCD update
    with lock:
        global page
        global screensaver

    # Create object for getting IP addresses of interfaces
    ipaddress = IPAddress()
    
    if screensaver==True:
        drawClear();
        return

    if page == 0:
        drawText(0,0,sd_image_version)
        drawText(0,14,"Serial: " + serial_num,True)

    if page == 1:
    # Get uptime
        with open('/proc/uptime', 'r') as f:
            seconds = float(f.readline().split()[0])
        r.set('uptime', seconds)

        drawText(0,0,datetime.now().strftime('%b %d %H:%M'))
        drawText(0,14,'Uptime %.2f days' % (seconds / 86400),True)

    # Now display the appropriate LCD page
    if page == 2:
    # Update ethernet
        eth0ip = ipaddress.get_ip_address('eth0')
        r.set("eth:active", int(bool(eth0ip)))
        r.set("eth:ip", eth0ip)

        wlan0ip = ipaddress.get_ip_address('wlan0')
        r.set("wlan:active", int(bool(wlan0ip)))

        if eval(r.get("eth:active")):
            drawText(0,0,"Ethernet: YES")
            drawText(0,14,r.get("eth:ip"),True)
        #elif eval(r.get("wlan:active")) or eval(r.get("gsm:active")):
        #    page += 1
        else:
            drawText(0,0,"Ethernet:")
            drawText(0,14,"NOT CONNECTED",True)

    if page == 3:
    # Update wifi
        wlan0ip = ipaddress.get_ip_address('wlan0')
        
        r.set("wlan:active", int(bool(wlan0ip)))
        r.set("wlan:ip", wlan0ip)

        signallevel = 0
        if wlan0ip:
            # wlan link status
            with open('/proc/net/wireless', 'r') as f:
                wireless = f.readlines()
                signals = [x.split()[3] for x in wireless if x.strip().startswith('wlan0')]
            if signals:
                signallevel = signals[0].partition('.')[0]
            if str(signallevel).startswith('-'): # Detect the alternate signal strength reporting via dBm
                signallevel = 2 * (int(signallevel) + 100) # Convert to percent
            r.set("wlan:signallevel", signallevel)

        if eval(r.get("wlan:active")):
            if int(r.get("wlan:signallevel")) > 0:
                drawText(0,0,"WiFi: YES  "+r.get("wlan:signallevel")+"%")
            else:
                drawText(0,0,"WiFi: YES")

            drawText(0,14,r.get("wlan:ip"),True)
        else:
            drawText(0,0,"WiFi:")
            drawText(0,14,"NOT CONNECTED",True)
        oled.image(image)
        oled.show()

    if page == 4:
        ap0ip = ipaddress.get_ip_address('ap0')
        
        r.set("ap0:active", int(bool(ap0ip)))
        r.set("ap0:ip", ap0ip)

        if eval(r.get("ap0:active")):
            drawText(0,0,"WiFi AP: YES")
            drawText(0,14,r.get("ap0:ip"),True)
        else:
            drawText(0,0,"WiFi AP:")
            drawText(0,14,"DISABLED",True)
        oled.image(image)
        oled.show()

    if page == 5:
        ret = subprocess.call(wifiAP_status, shell=True)       
        if ret == 0:
            drawText(0,0,"Disable WiFi AP?")
        else: 
            drawText(0,0,"Enable WiFi AP?")
            
        drawText(0,14,"Y press & hold",True)

    if page == 6:
    # Update Hi-Link 3G Dongle - connects on eth1
        if ipaddress.get_ip_address("eth1") and gsmhuaweistatus.is_hilink(hilink_device_ip):
            gsm_connection_status = gsmhuaweistatus.return_gsm_connection_status(hilink_device_ip)
            r.set("gsm:connection", gsm_connection_status[0])
            r.set("gsm:signal", gsm_connection_status[1])
            r.set("gsm:active", 1)
        else:
            r.set("gsm:active", 0)

        if eval(r.get("gsm:active")):
            drawText(0,0,r.get("gsm:connection"))
            drawText(0,14,r.get("gsm:signal"),True)
        elif eval(r.get("eth:active")) or eval(r.get("wlan:active")):
            page += 1
        else:
            drawText(0,0,"GSM:")
            drawText(0,14,"NO DEVICE",True)

    if page == 7:
        ret = subprocess.call(ssh_status, shell=True)
        if ret > 0:
            #ssh not running
            drawText(0,0,"SSH Enable?")
        else:
            #ssh not running
            drawText(0,0,"SSH Disable?")

        drawText(0,14,"Y press & hold",True)

    if page == 8:
        drawText(0,0,"Shutdown?")
        drawText(0,14,"Y press & hold",True)


class IPAddress:
    def __init__(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    def get_ip_address(self, ifname):
        try:
            return socket.inet_ntoa(fcntl.ioctl(
                self.sock.fileno(),
                0x8915,  # SIOCGIFADDR
                struct.pack('256s', ifname[:15].encode())
            )[20:24])
        except Exception:
            return ''

def preShutdown():
    drawText(0,0,"Shutdown?")
    drawText(0,14,"Hold 5 secs",True)

def shutdown():
    drawText(0,0,"emonPi Shutdown")
    drawText(0,14,"SHUTDOWN NOW!",True)
    time.sleep(2)
    drawText(0,0,"Wait 30s...")
    drawText(0,14,"Before Unplug!",True)
    time.sleep(4)
    # backlight zero must be the last call to the LCD to keep the backlight off

    subprocess.call(['sudo','halt'], shell=False)
    sys.exit(0)  # end script


def main():
    global page
    global screensaver
    global buttonPress_time
    global sd_image_version
    global serial_num
    
    global draw
    global oled
    global font
    global image

    # Initialise some redis variables
    r.set("gsm:active", 0)
    r.set("wlan:active", 0)
    r.set("eth:active", 0)

    # First set up logging
    if not uselogfile:
        loghandler = logging.StreamHandler()
    else:
        logfile = "/var/log/emonpilcd/emonpilcd.log"
        print("emonPiLCD logging to:", logfile)
        loghandler = logging.handlers.RotatingFileHandler(logfile,
                                                          mode='a',
                                                          maxBytes=1000 * 1024,
                                                          backupCount=1,
                                                         )

    loghandler.setFormatter(logging.Formatter('%(asctime)s %(levelname)s %(message)s'))
    logger.addHandler(loghandler)
    logger.setLevel(logging.INFO)

    logger.info("Starting emonPiLCD V" + version)

    # Now check the LCD and initialise the object
    # Scan I2C bus for LCD I2C addresses as defined in led_i2c, we have a
    # couple of models of LCD which have different addreses that are shipped
    # with emonPi. First I2C device to match address is used.
    global lcd
    for i2c_address in lcd_i2c:
        try:
            i2c = board.I2C()
            oled = adafruit_ssd1306.SSD1306_I2C(128, 32, i2c, addr=0x3C, reset=None)
        except OSError:
            # No LCD at this address, try the next...
            continue
        print("I2C LCD DETECTED Ox%s" % i2c_address)
        logger.info("I2C LCD DETECTED 0x%s" % i2c_address)
        # identify device as emonpi
        r.set("describe", "emonpi")
        break
    else:  # no break
        print("I2C LCD NOT DETECTED on either 0x" + str(lcd_i2c) + " ...exiting LCD script")
        logger.error("I2C LCD NOT DETECTED on either 0x" + str(lcd_i2c) + " ...exiting LCD script")
        # identify device as emonbase
        r.set("describe", "emonbase")
        sys.exit(0)


    oled.fill(0)
    oled.show()
    image = Image.new("1", (oled.width, oled.height))
    draw = ImageDraw.Draw(image)
    font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 11)


    # Read RaspberryPi serial number
    pi_serial = False
    try:
        f = open('/proc/cpuinfo','r')
        for line in f:
            if line[0:6]=='Serial':
                length=len(line)
                pi_serial = line[11:length-1]
        f.close()
    except:
        pi_serial = False
        logger.error("Cannot read RasPi serial number")

    if pi_serial:
        pi_serial_short = ""
        zero_flag = 1
        for i in range(0,len(pi_serial)):
            if pi_serial[i]!='0':
                zero_flag = 0
            if not zero_flag:
                pi_serial_short += pi_serial[i]
        serial_num = pi_serial_short.upper()
        logger.info("RasPi Serial Number: %s" % serial_num)
    else:
        serial_num = "Error"

    # ------------------------------------------------------------------------------------
    # Discover & display emonPi SD card image version
    # ------------------------------------------------------------------------------------

    boot_files = os.listdir('/boot')
    patterns = ['emonSD', 'emonpi']
    for filename, pattern in itertools.product(boot_files, patterns):
        if pattern in filename:
            sd_image_version = filename
            break
    else:
        sd_image_version = 'N/A'
        
    logger.info("SD card image build version: %s", sd_image_version)
    time.sleep(1)

    # Set up the buttons and install handlers

    # emonPi LCD push button Pin 16 GPIO 23
    # Uses gpiozero library to handle short and long press https://gpiozero.readthedocs.io/en/stable/api_input.html?highlight=button
    # push_btn = Button(23, pull_up=False, hold_time=5, bounce_time=0.1)
    # No bounce time increases response time but may result in switch bouncing...
    logger.info("Attaching push button interrupt...")
    try:
        push_btn = Button(4, pull_up=True, bounce_time=0.1, hold_time=5)
        #push_btn.when_pressed = buttonPress
        #push_btn.when_held = buttonPressLong
        
    except Exception:
        logger.error("Failed to attach LCD push button interrupt...")

    logger.info("Connecting to redis server...")
    # We wait here until redis has successfully started up
    while True:
        try:
            r.client_list()
            break
        except redis.ConnectionError:
            logger.error("waiting for redis-server to start...")
            time.sleep(1.0)
    logger.info("Connected to redis")

    logger.info("Connecting to MQTT Server: " + mqtt_host + " on port: " + str(mqtt_port) + " with user: " + mqtt_user)

    def on_message(client, userdata, msg):
        if msg.topic in mqtt_topics:
            r.set(mqtt_topics[msg.topic], msg.payload)

    def on_connect(client, userdata, flags, rc):
        if rc == 0:
            for topic in mqtt_topics:
                mqttc.subscribe(topic)

    mqttc = mqtt.Client()
    mqttc.on_message = on_message
    mqttc.on_connect = on_connect

    try:
        mqttc.username_pw_set(mqtt_user, mqtt_passwd)
        mqttc.reconnect_delay_set(min_delay=1, max_delay=120)
        mqttc.connect_async(mqtt_host, mqtt_port, 60)
        # Run MQTT background thread which handles reconnects
        mqttc.loop_start()
    except Exception:
        logger.error("Could not connect to MQTT")
    else:
        logger.info("Connected to MQTT")

    # time to show Build version
    # time.sleep(2)

    buttonPress_time = time.time()
    lcd_update_time = time.time()

    page = default_page
    updateLCD();

    btn_state = 0
    btn_press_timer = 0

    # Enter main loop
    while True:
        now = time.time()
        
        last_btn_state = btn_state
        btn_state = push_btn.is_pressed
        
        if btn_state != last_btn_state:
                
            if btn_state and not last_btn_state:
                btn_press_timer = now
                
            if last_btn_state and not btn_state:
                press_time = now - btn_press_timer
                if press_time<=1.0:
                    buttonPress()
                
            if not btn_state:
                btn_press_timer = 0
                draw.rectangle((108, 15, 120, 25), outline=0, fill=0)
                oled.image(image)
                oled.show()
                    
        if btn_state:
            press_time = math.floor(now - btn_press_timer)
            if page==5 or page == 7 or page == 8:
                if press_time>=1.0 and press_time<=5.0:
                    draw.rectangle((108, 15, 120, 25), outline=0, fill=0)
                    draw.text((110,14), str(press_time), font=font, fill=255)
                    oled.image(image)
                    oled.show()
                    if press_time==5.0:
                        buttonPressLong()
                
        
        if (now-buttonPress_time)>=59:
            screensaver = True

        #Update LCD in case it is left at a screen where values can change (e.g uptime etc)
        if (now-lcd_update_time)>=10.0:
            lcd_update_time = now
            updateLCD()
            
        time.sleep(0.1)

if __name__ == '__main__':
    main()
