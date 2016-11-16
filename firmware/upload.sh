#!/bin/bash

# Compile and upload direclty to the emonPi from the emonPi using Platformio

# make FS RW
echo "make file-system read-write"
rpi-rw

# stop openhub to free up serial port /dev/ttyAMA0
echo "stop emonhub"
sudo service emonhub stop

echo "Compile & upload using platformio"
sudo pio run -t upload

echo "start emonhub"
sudo service emonhub start

echo "put file system back to read-only"
# make FS readonly 
rpi-ro
