#!/bin/bash

# Force NTP time update

rpi-rw
sudo service ntp stop
sudo ntpd -q -g
sudo service ntp start
rpi-ro

