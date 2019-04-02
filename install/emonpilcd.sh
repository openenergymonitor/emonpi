#!/bin/bash

sudo apt-get install -y python-smbus i2c-tools python-rpi.gpio python-gpiozero
sudo pip install xmltodict

# Uncomment dtparam=i2c_arm=on
sudo sed -i "s/^#dtparam=i2c_arm=on/dtparam=i2c_arm=on/" /boot/config.txt
# Append line i2c-dev to /etc/modules
sudo sed -i -n '/i2c-dev/!p;$a i2c-dev' /etc/modules

# emonPiLCD folder is symlinked to /usr/share
# service expects python script to be at this location
sudo ln -s $usrdir/emonpi/lcd/ /usr/share/emonPiLCD

# emonPiLCD Logger
sudo mkdir /var/log/emonpilcd
# Permissions?
touch /var/log/emonpilcd/emonpilcd.log

# EmonBase / EmonPi flags are stored in /home/pi/data
cd
mkdir data

# Install and start emonPiLCD service
sudo ln -s /usr/share/emonPiLCD/emonPiLCD.service /lib/systemd/system
sudo systemctl daemon-reload
sudo systemctl enable emonPiLCD.service
sudo systemctl start emonPiLCD.service
