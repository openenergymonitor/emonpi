#!/bin/bash

# Compile on emonPi / RasPi using PlatofrmIO

# make FS RW
rpi-rw

# Compile using platformio
sudo pio run

# make FS readonly 
rpi-ro
