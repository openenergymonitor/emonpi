#!/bin/bash

echo "-------------------------------------------------------------"
echo "Main Update Script"
echo "-------------------------------------------------------------"

# -----------------------------------------------------------------
# Check environment
# -----------------------------------------------------------------

username="pi"
homedir="/home/$username"

echo "username: $username"
echo

echo "Checking environment:"

# Check that specified user directory exists
if [ -d $homedir ]; then 
    echo "- User directory $homedir found"
else 
    echo "- User directory $homedir not found (please ammend username)"
    exit 0
fi

emonSD_pi_env=0
# Check for pi user
pi=$(id -u pi)
if [ $pi ] && [ -d /home/pi ]; then
  echo "- pi user and pi user directory found"
  emonSD_pi_env=1
else 
  echo "- could not find pi user or pi user directory"
  emonSD_pi_env=0
fi

# Check emonSD version
image_version=$(ls /boot | grep emonSD)

if [ "$image_version" = "" ]; then
    echo "- Could not find emonSD version file"
    emonSD_pi_env=0
else 
    echo "- emonSD version: $image_version"
fi

if [ "$emonSD_pi_env" = "0" ]; then
    echo "- Assuming non emonSD install"
fi

# Check emoncms directory
if [ -d /var/www/emoncms ]; then
    emoncms_dir="/var/www/emoncms"
else
    if [ -d /var/www/html/emoncms ]; then
        emoncms_dir="/var/www/html/emoncms"
    else
        echo "emoncms directory not found"
        exit 0
    fi
fi
echo "- emoncms directory: $emoncms_dir"

echo
uid=`id -u`
echo "EUID: $uid"

if [ "$uid" = "0" ] ; then
    # update is being ran mistakenly as root, switch to user
    echo "update running as root: switching to $username user & restarting script"
    echo
    echo "**MANUAL SYSTEM REBOOT REQUIRED**"
    echo
    echo "Please reboot and run update again"
    su -c $0 $username
    exit
fi

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
/home/pi/emonpi/update/emoncms.sh $homedir $emonSD_pi_env $emoncms_dir
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
