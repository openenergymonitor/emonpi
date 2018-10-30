# Test gpiozero library for push and hold functionality
# https://gpiozero.readthedocs.io/en/stable/api_input.html?highlight=button

from gpiozero import Button
from signal import pause

print ("Push button test")

def buttonPressLong():
    print ("long press")

def buttonPress():
   print "short press"

push_btn = Button(23, pull_up=False, hold_time=5, bounce_time=0.1)

    
print "done attaching"
push_btn.when_pressed = buttonPress
push_btn.when_held = buttonPressLong
   

pause()
