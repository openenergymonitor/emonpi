from time import sleep
import RPi.GPIO as GPIO

press_count =0

GPIO.setmode(GPIO.BOARD)
GPIO.setup(16, GPIO.IN, pull_up_down = GPIO.PUD_DOWN)



def buttonPress(channel):
	global press_count
	print('Button press! ')
	press_count += 1






# Define a function to keep script running
def loop():
    raw_input()

press_num=0
GPIO.add_event_detect(16, GPIO.RISING, callback=buttonPress, bouncetime=200)



while 1:
	print(press_count)
	sleep(1)


	

GPIO.cleanup()