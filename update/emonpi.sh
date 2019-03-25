#!/bin/bash
echo "-------------------------------------------------------------"
echo "EmonPi Firmware Update"
echo "-------------------------------------------------------------"

sudo service emonhub stop

echo "Start ATmega328 serial upload using avrdude with latest.hex"

echo "Discrete Sampling"

echo "avrdude -c arduino -p ATMEGA328P -P /dev/ttyAMA0 -b 115200 -U flash:w:/home/pi/emonpi/firmware/compiled/latest.hex"

avrdude -c arduino -p ATMEGA328P -P /dev/ttyAMA0 -b 115200 -U flash:w:/home/pi/emonpi/firmware/compiled/latest.hex

sudo service emonhub start

