#!/bin/bash
source config.ini

sudo apt-get install -y python-smbus i2c-tools python-rpi.gpio python-gpiozero
sudo pip install xmltodict

# Uncomment dtparam=i2c_arm=on
sudo sed -i "s/^#dtparam=i2c_arm=on/dtparam=i2c_arm=on/" /boot/config.txt
# Append line i2c-dev to /etc/modules
sudo sed -i -n '/i2c-dev/!p;$a i2c-dev' /etc/modules

# emonPiLCD folder is symlinked to /usr/share
# service expects python script to be at this location
if [ ! -L /usr/share/emonPiLCD ]; then
    sudo ln -s $usrdir/emonpi/lcd/ /usr/share/emonPiLCD
fi


if [ ! -d /var/log/emonpilcd ]; then
    # emonPiLCD Logger
    sudo mkdir /var/log/emonpilcd
    # Permissions?
    sudo touch /var/log/emonpilcd/emonpilcd.log
fi

# EmonBase / EmonPi flags are stored in /home/pi/data
cd
if [ ! -d data ]; then mkdir data; fi

# Install and start emonPiLCD service
servicepath=/usr/share/emonPiLCD/emonPiLCD.service
$usrdir/emonpi/update/install_emoncms_service.sh $servicepath emonPiLCD
