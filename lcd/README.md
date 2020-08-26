# HD44780 LCD with PCF8574 I2c

I2C Address: 0x27 or 0x3f


# Enabling The I2C Port

**Tested on Raspbian**

The I2C ports need to be enabled in Raspbian before they can be used.

	$ sudo nano /boot/config.txt

Un-comment / add the line

	dtparam=i2c_arm=on

Edit kernel modules file:

	$ sudo nano /etc/modules

Add this line:

i2c-dev

Exit and save the file.

## Install the I2C utilities:

Usually pre-installed

		$ sudo apt-get install python-smbus i2c-tools

Enter "sudo reboot" to restart the pi and now the I2C pins will be available to use.


## Detect LCD on I2C bus and find out address

	$ sudo i2cdetect -y 1

Expected output is `0x27` or `0x3F` e.g.

```
pi@emonpi:~ $  sudo i2cdetect -y 1
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
00:          -- -- -- -- -- -- -- -- -- -- -- -- -- 
10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
20: -- -- -- -- -- -- -- 27 -- -- -- -- -- -- -- -- 
30: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
40: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
50: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
60: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
70: -- -- -- -- -- -- -- --   
```

Use port 0 for very old 256Mb RAM pi (rev1) - not recommended to use this pi version

# Install emonPiLCD python script

```
sudo apt-get update
sudo apt-get install python3-smbus i2c-tools python3-rpi.gpio python3-pip redis-server python3-gpiozero -y
sudo pip3 install redis paho-mqtt xmltodict requests
```

## Run as service

Run the shell script  
```
./install.sh
```
