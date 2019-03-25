#!/bin/bash

# emonPi update for use with service-runner add following entry to crontab:
# * * * * * /home/pi/emonpi/service-runner >> /var/log/service-runner.log 2>&1

echo "-------------------------------------------------------------"

# Clear log update file
cat /dev/null >  /home/pi/data/emonpiupdate.log

echo "Starting emonPi Update >"
echo "via service-runner-update.sh"
echo "Service Runner update script V1.1.1"
echo "EUID: $EUID"
argument=$1
echo "Argument: "$argument
# Date and time
date
echo "-------------------------------------------------------------"
# Check emonSD base image is minimum required release date, else don't update
image_version=$(ls /boot | grep emonSD)
echo "emonSD version: $image_version"
if [ "$image_version" == "emonSD-07Nov16" ] || [ "$image_version" == "emonSD-03May16" ] || [ "$image_version" == "emonSD-26Oct17" ] || [ "$image_version" == "emonSD-13Jun18" ] || [ "$image_version" == "emonSD-30Oct18" ]; then
  echo "emonSD base image check passed...continue update"
else
  echo "ERROR: emonSD base image old or undefined...update will not continue"
  echo "See latest verson: https://github.com/openenergymonitor/emonpi/wiki/emonSD-pre-built-SD-card-Download-&-Change-Log"
  echo "Stopping update"
  exit
fi
echo "-------------------------------------------------------------"

# Stop emonPi LCD servcice
# sudo service emonPiLCD stop

# Display update message on LCD
# sudo /home/pi/emonpi/lcd/./emonPiLCD_update.py

# make file system read-write
if [ -f /usr/bin/rpi-rw ]; then
  rpi-rw
fi

# Pull in latest emonpi repo before then running updated update scripts
echo "git pull /home/pi/emonpi"
cd /home/pi/emonpi
sudo rm -rf hardware/emonpi/emonpi2c/
git branch
git status
git pull

# Run update in main update script
/home/pi/emonpi/update/main.sh
