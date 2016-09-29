#!/bin/bash

# Compile and upload direclty to the emonPi from the emonPi using Platformio

# make FS RW
rpi-rw

# stop openhub to free up serial port /dev/ttyAMA0

sudo service emonhub stop

# Compile & upload using platformio
sudo pio run -t upload

sudop service emonhub start

# make FS readonly 
rpi-ro
