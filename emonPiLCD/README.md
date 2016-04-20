# HD44780 LCD with PCF8574 I2c 
I2C Address: 0x27


#Enabling The I2C Port
The I2C ports need to be enabled in Raspbian before they can be used.

## Edit the modules file

 	$ sudo nano /etc/modules

Add these lines:

i2c-bcm2708
i2c-dev

Exit and save the file


## Edit the modules blacklist file:

	sudo nano /etc/modprobe.d/raspi-blacklist.conf

Add a '#' character to this line so it commented out:

		#blacklist i2c-bcm2708

Exit and save the file.

​
## Finally install the I2C utilities:

		$ sudo apt-get install python-smbus i2c-tools

Enter "sudo reboot" to restart the pi and now the I2C pins will be available to use.


## Detect LCD on I2C bus and find out address

 	$ sudo i2cdetect -y 1

Use port 1 for 512Mb RAM pi (rev2) or port 0 for older 256Mb RAM pi (rev1)
