# from http://www.pi-supply.com/pi-supply-switch-v1-1-code-examples/

# Import the modules to send commands to the system and access GPIO pins
from subprocess import call
import RPi.GPIO as gpio
 
# Define a function to keep script running
def loop():
    raw_input()
 
# Define a function to run when an interrupt is called
def shutdown(pin):
    call('halt', shell=False)
 
gpio.setmode(gpio.BOARD) # Set pin numbering to board numbering
gpio.setup(17, gpio.IN) # Set up pin 7 as an input
gpio.add_event_detect(17, gpio.RISING, callback=shutdown, bouncetime=200) # Set up an interrupt to look for button presses
 
loop() # Run the loop function to keep script running