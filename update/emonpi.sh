#!/bin/bash
echo "-------------------------------------------------------------"
echo "EmonPi Firmware Update"
echo "-------------------------------------------------------------"
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
usrdir=${DIR/\/emonpi\/update/}

sudo service emonhub stop

echo "Start ATmega328 serial upload using avrdude with latest.hex"

echo "Discrete Sampling"

echo "avrdude -c arduino -p ATMEGA328P -P /dev/ttyAMA0 -b 115200 -U flash:w:$usrdir/emonpi/firmware/compiled/latest.hex"

avrdude -c arduino -p ATMEGA328P -P /dev/ttyAMA0 -b 115200 -U flash:w:$usrdir/emonpi/firmware/compiled/latest.hex

sudo service emonhub start

