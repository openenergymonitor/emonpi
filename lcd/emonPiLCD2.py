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
max_number_pages = 11
page = default_page

sd_image_version = ''

sshConfirm = False
shutConfirm = False

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

oled_last = True

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
    logger.info("Mode button LONG press")

    if sshConfirm:
        ret = subprocess.call(ssh_status, shell=True)
        if ret > 0:
            #ssh not running, enable & start it
            subprocess.call(ssh_enable, shell=True)
            subprocess.call(ssh_start, shell=True)
            logger.info("SSH Enabled")
            drawText(0,0,'SSH Enabled')
            drawText(0,14,'Change password!',True)
        else:
            #disable ssh
            subprocess.call(ssh_disable, shell=True)
            subprocess.call(ssh_stop, shell=True)
            logger.info("SSH Disabled")
            drawText(0,0,'SSH Disabled')
            drawText(0,14,'',True)

    elif shutConfirm:
        logger.info("Shutting down")
        shutdown()


def buttonPress():
    global page
    now = time.time()

    page += 1
    if page > max_number_pages:
        page = 0
    buttonPress_time = now
    
    logger.info("Mode button SHORT press")
    logger.info("Page: " + str(page))
    updateLCD()


def updateLCD():
    # Lock thread to avoid LCD being corrupted if a button press interrupt interrupts the LCD update
    with lock:
        global page
        global sshConfirm
        global shutConfirm

    # Create object for getting IP addresses of interfaces
    ipaddress = IPAddress()

    if not page == 9:
        sshConfirm = False
    if not page == 11:
        shutConfirm = False

    # Now display the appropriate LCD page
    if page == 0:
    # Update ethernet
        eth0ip = ipaddress.get_ip_address('eth0')
        r.set("eth:active", int(bool(eth0ip)))
        r.set("eth:ip", eth0ip)

        wlan0ip = ipaddress.get_ip_address('wlan0')
        r.set("wlan:active", int(bool(wlan0ip)))

        if eval(r.get("eth:active")):
            drawText(0,0,"Ethernet: YES")
            drawText(0,14,r.get("eth:ip"),True)
        elif eval(r.get("wlan:active")) or eval(r.get("gsm:active")):
            page += 1
        else:
            drawText(0,0,"Ethernet:")
            drawText(0,14,"NOT CONNECTED",True)

    if page == 1:
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
                if r.get("wlan:ip") == "192.168.42.1":
                    drawText(0,0,"WiFi: AP MODE")
                else:
                    drawText(0,0,"WiFi: YES")

            drawText(0,14,r.get("wlan:ip"),True)
        elif eval(r.get("gsm:active")) or eval(r.get("eth:active")):
            page += 1
        else:
            drawText(0,0,"WiFi:")
            drawText(0,14,"NOT CONNECTED",True)
        oled.image(image)
        oled.show() 

    if page == 2:
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

    if page == 3:
        if r.get("feed1") is not None:
            drawText(0,0,feed1_name + ':'  + r.get("feed1") + feed1_unit)
        else:
            drawText(0,0,feed1_name + ':'  + "---")

        if r.get("feed2") is not None:
            drawText(0,14,feed2_name + ':'  + r.get("feed2") + feed2_unit,True)
        else:
            drawText(0,14,feed2_name + ':'  + "---",True)

    elif page == 4:
        vrms = r.get("vrms")
        pulse = r.get("pulse")
        if vrms is not None and pulse is not None:
            drawText(0,0,'VRMS: ' + vrms + 'V')
            drawText(0,14,'Pulse: ' + pulse + 'p',True)
        else:
            drawText(0,0,'Connecting...')
            drawText(0,14,'Please Wait',True)
            page += 1

    elif page == 5:
        temp1 = r.get('temp1')
        temp2 = r.get('temp2')
        if temp1 is not None and temp2 is not None:
            drawText(0,0,'Temp 1: ' + temp1 + 'C')
            drawText(0,14,'Temp 2: ' + temp2 + 'C',True)
        else:
            drawText(0,0,'Connecting...')
            drawText(0,14,'Please Wait',True)
            page += 1

    elif page == 6:
    # Get uptime
        with open('/proc/uptime', 'r') as f:
            seconds = float(f.readline().split()[0])
        r.set('uptime', seconds)

        drawText(0,0,datetime.now().strftime('%b %d %H:%M'))
        drawText(0,14,'Uptime %.2f days' % (seconds / 86400),True)

    elif page == 7:
        drawText(0,0,sd_image_version)
        drawText(0,14,"Serial: " + serial_num,True)

    elif page == 8:
        ret = subprocess.call(ssh_status, shell=True)
        if ret > 0:
            #ssh not running
            drawText(0,0,"SSH Enable?")
        else:
            #ssh not running
            drawText(0,0,"SSH Disable?")

        drawText(0,14,"Y press & hold",True)
        sshConfirm = False

    elif page == 9:
        sshConfirm = True

    elif page == 10:
        drawText(0,0,"Shutdown?")
        drawText(0,14,"Y press & hold",True)
        shutConfirm = False
    elif page == 11:
        drawText(0,0,"Shutdown?",True)
        shutConfirm = True


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
    
    drawText(0,0,sd_image_version)
    drawText(0,14,"Serial: " + serial_num,True)
    
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
        push_btn.when_pressed = buttonPress
        push_btn.when_held = buttonPressLong
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

    page = default_page

    # Enter main loop
    while True:
        now = time.time()

        #Update LCD in case it is left at a screen where values can change (e.g uptime etc)
        updateLCD()
        time.sleep(lcd_update_sec)

if __name__ == '__main__':
    main()
