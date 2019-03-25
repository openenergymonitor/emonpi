#!/bin/bash

echo "-------------------------------------------------------------"
echo "Main Update Script"
echo "-------------------------------------------------------------"

# sudo apt-get update

# Ensure rpi gpio is latest version and gpiozero is installed, required for V2.2.0 LCD script
# rng-tools used to speed up entropy generation https://community.openenergymonitor.org/t/cant-connect-to-local-emoncms/9498/7
# sudo apt-get install python-gpiozero python-rpi.gpio rng-tools -y
# sudo pip install paho-mqtt --upgrade

# -----------------------------------------------------------------

if [ -d /home/pi/RFM2Pi ]; then
    echo "git pull /home/pi/RFM2Pi"
    cd /home/pi/RFM2Pi
    git branch
    git status
    git pull
    echo
fi

if [ -d /home/pi/usefulscripts ]; then
    echo "git pull /home/pi/usefulscripts"
    cd /home/pi/usefulscripts
    git branch
    git status
    git pull
    echo
fi

if [ -d /home/pi/huawei-hilink-status ]; then
    echo "git pull /home/pi/huawei-hilink-status"
    cd /home/pi/huawei-hilink-status
    git branch
    git status
    git pull
    echo
fi

if [ -d /home/pi/oem_openHab ]; then
    echo "git pull /home/pi/oem_openHab"
    cd /home/pi/oem_openHab
    git branch
    git status
    git pull
    echo
fi

if [ -d /home/pi/oem_node-red ]; then
    echo "git pull /home/pi/oem_node-red"
    cd /home/pi/oem_node-red
    git branch
    git status
    git pull
    echo
fi

# -----------------------------------------------------------------
# Firmware update
# -----------------------------------------------------------------
# Check if we have an emonpi LCD connected, 
# if we do assume EmonPi hardware else assume RFM69Pi
lcd27=$(sudo /home/pi/emonpi/lcd/emonPiLCD_detect.sh 27 1)
lcd3f=$(sudo /home/pi/emonpi/lcd/emonPiLCD_detect.sh 3f 1)

if [ $lcd27 == 'True' ] || [ $lcd3f == 'True' ]; then
    hardware="EmonPi"
else
    hardware="rfm2pi"
fi
echo "Hardware detected: $hardware"

# Run relevant hardware update script
# if [ "$hardware" == "rfm2pi" ]; then
  # /home/pi/emonpi/update/rfm69pi.sh
# else
  # /home/pi/emonpi/update/emonpi.sh
# fi

# -----------------------------------------------------------------

echo
echo "Start emonhub update script:"
# Run emonHub update script to update emonhub.conf nodes
/home/pi/emonpi/update/emonhub.sh
echo

echo "Start emoncms update:"
# Run emoncms update script to pull in latest emoncms & emonhub updates
/home/pi/emonpi/update/emoncms.sh
echo

exit 0

echo
# Wait for update to finish
echo "Starting emonPi LCD service.."
sleep 5
sudo service emonPiLCD restart
echo

if [ -f /usr/bin/rpi-ro ]; then
  rpi-ro
fi

date
echo
printf "\n...................\n"
printf "emonPi update done\n" # this text string is used by service runner to stop the log window polling, DO NOT CHANGE!

printf "restarting service-runner\n"
# old service runner
killall service-runner
# new service runner
sudo systemctl restart service-runner.service 
