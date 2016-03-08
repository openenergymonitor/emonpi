#!/bin/bash

# emonPi update for use with service-runner add following entry to crontab:
# * * * * * /home/pi/emonpi/service-runner >> /var/log/service-runner.log 2>&1 

# Make FS RW
rpi-rw

# Clear log update file
cat /dev/null >  /home/pi/data/emonpiupdate.log

# Stop emonPi LCD servcice
sudo service emonPiLCD stop

# Display update message on LCD
sudo /home/pi/emonpi/lcd/./emonPiLCD_update.py


echo "Starting emonPi Update >"
echo
# Date and time
date
echo

echo "git pull /home/pi/emonpi"
cd /home/pi/emonpi
git pull

echo "git pull /home/pi/RFM2Pi"
cd /home/pi/RFM2Pi
git pull

echo "git pull /home/pi/emonhub"
cd /home/pi/emonhub
git pull

if [ -d /home/pi/oem_openHab ]; then
    echo "git pull /home/pi/oem_openHab"
    cd /home/pi/oem_openHab
    git pull
fi

if [ -d /home/pi/oem_node ]; then
    echo "git pull /home/pi/oem_node-red"
    cd /home/pi/oem_node-red
    git pull
fi

if [ -d /home/pi/usefulscripts ]; then
    echo "git pull /home/pi/usefulscripts"
    cd /home/pi/usefulscripts
    git pull
fi

if [ -d /home/pi/backup ]; then
    echo "git pull /home/pi/backup"
    cd /home/pi/backup
    git pull
fi

echo
echo "Start emonPi Atmega328 firmware update:"
# Run emonPi update script to update firmware on Atmega328 on emonPi Shield using avrdude
/home/pi/emonpi/emonpiupdate
echo

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
sleep 30
echo "Start emonPi LCD service"

sudo service emonPiLCD start
echo
rpi-ro
date
echo
printf "\n...................\n"
printf "emonPi update done\n" # this text string is used by service runner to stop the log window polling, DO NOT CHANGE!


