#!/usr/bin/env python

# from http://www.pi-supply.com/pi-supply-switch-v1-1-code-examples/

# Import the modules to send commands to the system and access GPIO pins
from subprocess import call
import RPi.GPIO as gpio

print 'start emonPi shutdown script on GPIO 16' 
 
# Define a function to run when an interrupt is called
def shutdown(pin):
    call('halt', shell=False)
 
gpio.setmode(gpio.BOARD) # Set pin numbering to board numbering
gpio.setup(11, gpio.IN) # Set up pin 11 GPIO 17 as an input
gpio.add_event_detect(11, gpio.RISING, callback=shutdown, bouncetime=200) # Set up an interrupt to look for button presses
 
while 1:
 raw_input()

GPIO.cleanup()
