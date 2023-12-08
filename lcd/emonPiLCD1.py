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
import lcddriver
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
mqtt_topics = {config.get('mqtt', 'mqtt_emonpi_topic'): 'basedata',
               config.get('mqtt', 'mqtt_temp1_topic'): 'temp1',
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
lcd_i2c = ['27', '3f']
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
uselogfile = config.getboolean('general', 'uselogfile')
logger = logging.getLogger("emonPiLCD")

#ssh enable/disable/check commands
ssh_enable = "sudo systemctl enable ssh > /dev/null"
ssh_start = "sudo systemctl start ssh > /dev/null"
ssh_disable = "sudo systemctl disable ssh > /dev/null"
ssh_stop = "sudo systemctl stop ssh > /dev/null"
ssh_status = "sudo systemctl status ssh > /dev/null"

def buttonPressLong():
    logger.info("Mode button LONG press")

    if sshConfirm:
        ret = subprocess.call(ssh_status, shell=True)
        if ret > 0:
            #ssh not running, enable & start it
            subprocess.call(ssh_enable, shell=True)
            subprocess.call(ssh_start, shell=True)
            logger.info("SSH Enabled")
            lcd[0] = 'SSH Enabled'
            lcd[1] = 'Change password!'
        else:
            #disable ssh
            subprocess.call(ssh_disable, shell=True)
            subprocess.call(ssh_stop, shell=True)
            logger.info("SSH Disabled")
            lcd[0] = 'SSH Disabled'
            lcd[1] = ''

    elif shutConfirm:
        logger.info("Shutting down")
        shutdown()
    else:
        lcd.backlight = not lcd.backlight


def buttonPress():
    global page

    now = time.time()

    if lcd.backlight:
        page += 1
    if page > max_number_pages:
        page = 0
    buttonPress_time = now
    if not lcd.backlight:
        lcd.backlight = 1
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
            lcd[0] = "Ethernet: YES"
            lcd[1] = r.get("eth:ip")
        elif eval(r.get("wlan:active")) or eval(r.get("gsm:active")):
            page += 1
        else:
            lcd[0] = "Ethernet:"
            lcd[1] = "NOT CONNECTED"

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
                lcd[0] = "WiFi: YES  " + r.get("wlan:signallevel") + "%"
            else:
                if r.get("wlan:ip") == "192.168.42.1":
                    lcd[0] = "WiFi: AP MODE"
                else:
                    lcd[0] = "WiFi: YES"

            lcd[1] = r.get("wlan:ip")
        elif eval(r.get("gsm:active")) or eval(r.get("eth:active")):
            page += 1
        else:
            lcd[0] = "WiFi:"
            lcd[1] = "NOT CONNECTED"

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
            lcd[0] = r.get("gsm:connection")
            lcd[1] = r.get("gsm:signal")
        elif eval(r.get("eth:active")) or eval(r.get("wlan:active")):
            page += 1
        else:
            lcd[0] = "GSM:"
            lcd[1] = "NO DEVICE"

    if page == 3:
        if r.get("feed1") is not None:
            lcd[0] = feed1_name + ':'  + r.get("feed1") + feed1_unit
        else:
            lcd[0] = feed1_name + ':'  + "---"

        if r.get("feed2") is not None:
            lcd[1] = feed2_name + ':'  + r.get("feed2") + feed2_unit
        else:
            lcd[1] = feed2_name + ':'  + "---"

    elif page == 4:
        basedata = r.get("basedata")
        vrms = r.get("vrms")
        pulse = r.get("pulse")
        if basedata is not None:
            basedata = basedata.split(",")
            lcd[0] = 'VRMS: ' + basedata[3] + "V"
            lcd[1] = 'Pulse: ' + basedata[10] + "p"
        elif vrms is not None and pulse is not None:
            lcd[0] = 'VRMS: ' + vrms + 'V'
            lcd[1] = 'Pulse: ' + pulse + 'p'
        else:
            lcd[0] = 'Connecting...'
            lcd[1] = 'Please Wait'
            page += 1

    elif page == 5:
        basedata = r.get("basedata")
        temp1 = r.get('temp1')
        temp2 = r.get('temp2')
        if basedata is not None:
            basedata = basedata.split(",")
            lcd[0] = 'Temp 1: ' + basedata[4] + "\0C"
            lcd[1] = 'Temp 2: ' + basedata[5] + "\0C"
        elif temp1 is not None and temp2 is not None:
            lcd[0] = 'Temp 1: ' + temp1 + '\0C'
            lcd[1] = 'Temp 2: ' + temp2 + '\0C'
        else:
            lcd[0] = 'Connecting...'
            lcd[1] = 'Please Wait'
            page += 1

    elif page == 6:
    # Get uptime
        with open('/proc/uptime', 'r') as f:
            seconds = float(f.readline().split()[0])
        r.set('uptime', seconds)

        lcd[0] = datetime.now().strftime('%b %d %H:%M')
        lcd[1] = 'Uptime %.2f days' % (seconds / 86400)

    elif page == 7:
        lcd[0] = sd_image_version
        lcd[1] = "Serial: " + serial_num

    elif page == 8:
        ret = subprocess.call(ssh_status, shell=True)
        if ret > 0:
            #ssh not running
            lcd[0] = "SSH Enable?"
        else:
            #ssh not running
            lcd[0] = "SSH Disable?"

        lcd[1] = "Y press & hold"
        sshConfirm = False

    elif page == 9:
        sshConfirm = True

    elif page == 10:
        lcd[0] = "Shutdown?"
        lcd[1] = "Y press & hold"
        shutConfirm = False
    elif page == 11:
        lcd[0] = "Shutdown?"
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
    lcd[0] = "Shutdown?"
    lcd[1] = "Hold 5 secs"

def shutdown():
    lcd.backlight = 1
    lcd[0] = "emonPi Shutdown"
    lcd[1] = "SHUTDOWN NOW!"
    time.sleep(2)
    lcd[0] = "Wait 30s..."
    lcd[1] = "Before Unplug!"
    time.sleep(4)
    # backlight zero must be the last call to the LCD to keep the backlight off
    lcd.backlight = 0
    subprocess.call(['sudo','halt'], shell=False)
    sys.exit(0)  # end script


def main():
    global page
    global sd_image_version
    global serial_num

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
            lcd = lcddriver.lcd(int(i2c_address, 16))
            lcd.backlight = 1
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

    # This is a row-major bitmap of a character.
    degree_sign = [ 6, 9, 9, 6, 0, 0, 0, 0 ]
    lcd.lcd_create_char(0, degree_sign)

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

    lcd[0] = sd_image_version
    lcd[1] = "Serial: " + serial_num
    logger.info("SD card image build version: %s", sd_image_version)
    time.sleep(5)

    # Set up the buttons and install handlers

    # emonPi LCD push button Pin 16 GPIO 23
    # Uses gpiozero library to handle short and long press https://gpiozero.readthedocs.io/en/stable/api_input.html?highlight=button
    # push_btn = Button(23, pull_up=False, hold_time=5, bounce_time=0.1)
    # No bounce time increases response time but may result in switch bouncing...
    logger.info("Attaching push button interrupt...")
    try:
        push_btn = Button(23, pull_up=False, hold_time=5)
        push_btn.when_pressed = buttonPress
        push_btn.when_held = buttonPressLong
    except Exception:
        logger.error("Failed to attach LCD push button interrupt...")

    logger.info("Attaching shutdown button interrupt...")
    try:
        shut_btn = Button(17, pull_up=False, hold_time=5)
        shut_btn.when_pressed = preShutdown
        shut_btn.when_held = shutdown
    except Exception:
        logger.error("Failed to attach shutdown button interrupt...")

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
    time.sleep(2)

    buttonPress_time = time.time()

    if not backlight_timeout:
        lcd.backlight = 1

    page = default_page

    # Enter main loop
    while True:
        # turn backlight off after backlight_timeout seconds
        now = time.time()
        if backlight_timeout and now - buttonPress_time > backlight_timeout and lcd.backlight:
            lcd.backlight = 0

        #Update LCD in case it is left at a screen where values can change (e.g uptime etc)
        updateLCD()
        time.sleep(lcd_update_sec)

if __name__ == '__main__':
    main()
