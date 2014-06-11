import RPi.GPIO as GPIO

GPIO.setmode(GPIO.BOARD)

GPIO.setup(16, GPIO.IN, pull_up_down = GPIO.PUD_DOWN)

def printFunction(channel):

	print('Button 1 pressed!')

	print('Note how the bouncetime affects the button press')


# Define a function to keep script running
def loop():
    raw_input()

GPIO.add_event_detect(16, GPIO.RISING, callback=printFunction, bouncetime=200)

loop()

	

GPIO.cleanup()