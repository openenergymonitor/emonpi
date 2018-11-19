#!/bin/bash

# emonPi update for use with service-runner add following entry to crontab:
# * * * * * /home/pi/emonpi/service-runner >> /var/log/service-runner.log 2>&1


echo "#############################################################"


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
echo "#############################################################"
echo
# Check emonSD base image is minimum required release date, else don't update
image_version=$(ls /boot | grep emonSD)
echo "emonSD version: $image_version"
echo

if [ "$image_version" == "emonSD-07Nov16" ] || [ "$image_version" == "emonSD-03May16" ] || [ "$image_version" == "emonSD-26Oct17" ] || [ "$image_version" == "emonSD-13Jun18" ] || [ "$image_version" == "emonSD-30Oct18" ]; then
  echo "emonSD base image check passed...continue update"
else
  echo "ERROR: emonSD base image old or undefined...update will not continue"
  echo "See latest verson: https://github.com/openenergymonitor/emonpi/wiki/emonSD-pre-built-SD-card-Download-&-Change-Log"
  echo "Stopping update"
  exit
fi
echo
echo "#############################################################"

# Stop emonPi LCD servcice
sudo service emonPiLCD stop

# Display update message on LCD
sudo /home/pi/emonpi/lcd/./emonPiLCD_update.py

# make file system read-write
rpi-rw

echo "git pull /home/pi/emonpi"
cd /home/pi/emonpi
sudo rm -rf hardware/emonpi/emonpi2c/
git branch
git status
git pull

echo "git pull /home/pi/RFM2Pi"
cd /home/pi/RFM2Pi
git branch
git status
sudo chown -R pi:pi .git
git pull

echo "git pull /home/pi/emonhub"
cd /home/pi/emonhub
git branch
git status
git pull

if [ -d /home/pi/oem_openHab ]; then
    echo "git pull /home/pi/oem_openHab"
    cd /home/pi/oem_openHab
    git branch
    git status
    git pull
fi

if [ -d /home/pi/oem_node ]; then
    echo "git pull /home/pi/oem_node-red"
    cd /home/pi/oem_node-red
    git branch
    git status
    git pull
fi

if [ -d /home/pi/usefulscripts ]; then
    echo "git pull /home/pi/usefulscripts"
    cd /home/pi/usefulscripts
    sudo chown -R pi:pi .git
    git branch
    git status
    git pull
fi

if [ -d /home/pi/huawei-hilink-status ]; then
    echo "git pull /home/pi/huawei-hilink-status"
    cd /home/pi/huawei-hilink-status
    git branch
    git status
    git pull
fi

echo

# if passed argument from Emoncms admin is rfm69pi then run rfm69pi update instead of emonPi
if [ "$argument" == "rfm69pi" ]; then
  echo "Running RFM69Pi firmware update:"
  /home/pi/emonpi/rfm69piupdate.sh
  echo
else
  echo "Start emonPi Atmega328 firmware update:"
  # Run emonPi update script to update firmware on Atmega328 on emonPi Shield using avrdude
  /home/pi/emonpi/emonpiupdate
  echo
fi

echo
echo "Start emonhub update script:"
# Run emonHub update script to update emonhub.conf nodes
/home/pi/emonpi/emonhubupdate
echo

echo "Start emoncms update:"
# Run emoncms update script to pull in latest emoncms & emonhub updates
/home/pi/emonpi/emoncmsupdate
echo

echo
# Wait for update to finish
echo "Starting emonPi LCD service.."
sleep 5
sudo service emonPiLCD restart
echo
rpi-ro
date
echo
printf "\n...................\n"
printf "emonPi update done\n" # this text string is used by service runner to stop the log window polling, DO NOT CHANGE!

printf "restarting service-runner\n"
# old service runner
killall service-runner
# new service runner
sudo systemctl restart service-runner.service 
