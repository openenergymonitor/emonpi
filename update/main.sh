#!/bin/bash

echo "-------------------------------------------------------------"
echo "Main Update Script"
echo "-------------------------------------------------------------"
# -----------------------------------------------------------------
# Check environment
# -----------------------------------------------------------------

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
usrdir=${DIR/\/emonpi\/update/}

type=$1
firmware=$2

datestr=$(date)

echo "Date:" $datestr
echo "EUID: $EUID"
echo "usrdir: $usrdir"
echo "type: $type"
echo "firmware: $firmware"

echo "Checking environment:"

emonSD_pi_env=0
# Check for pi user
pi=$(id -u pi)
if [ $pi ]; then
  echo "- pi user found"
  emonSD_pi_env=1
else 
  echo "- could not find pi user"
  emonSD_pi_env=0
fi

# Check emonSD version
image_version=$(ls /boot | grep emonSD)
if [ "$image_version" = "" ]; then
    echo "- Could not find emonSD version file"
    emonSD_pi_env=0
fi

if [ "$emonSD_pi_env" = "0" ]; then
    echo "- Assuming non emonSD install"
fi

echo
uid=`id -u`
echo "EUID: $uid"

if [ "$uid" = "0" ] ; then
    # update is being ran mistakenly as root, switch to user
    echo "update running as root, switch to user"
    exit 0
fi

if [ "$emonSD_pi_env" = "1" ]; then
    # Check if we have an emonpi LCD connected, 
    # if we do assume EmonPi hardware else assume RFM69Pi
    lcd27=$(sudo $usrdir/emonpi/lcd/emonPiLCD_detect.sh 27 1)
    lcd3f=$(sudo $usrdir/emonpi/lcd/emonPiLCD_detect.sh 3f 1)

    if [ $lcd27 == 'True' ] || [ $lcd3f == 'True' ]; then
        hardware="EmonPi"
    else
        hardware="rfm2pi"
    fi
    echo "Hardware detected: $hardware"
    
    # Stop emonPi LCD servcice
    echo "Stopping emonPiLCD service"
    sudo service emonPiLCD stop

    # Display update message on LCD
    echo "Display update message on LCD"
    sudo $usrdir/emonpi/lcd/./emonPiLCD_update.py
fi

echo "-------------------------------------------------------------"

sudo apt-get update

# Ensure rpi gpio is latest version and gpiozero is installed, required for V2.2.0 LCD script
# rng-tools used to speed up entropy generation https://community.openenergymonitor.org/t/cant-connect-to-local-emoncms/9498/7
sudo apt-get install python-gpiozero python-rpi.gpio rng-tools -y
sudo pip install paho-mqtt --upgrade

echo "-------------------------------------------------------------"

# -----------------------------------------------------------------

if [ "$type" == "all" ]; then

    if [ -d $usrdir/RFM2Pi ]; then
        echo "git pull $usrdir/RFM2Pi"
        cd $usrdir/RFM2Pi
        git branch
        git status
        git pull
        echo
    fi

    if [ -d $usrdir/usefulscripts ]; then
        echo "git pull $usrdir/usefulscripts"
        cd $usrdir/usefulscripts
        git branch
        git status
        git pull
        echo
    fi

    if [ -d $usrdir/huawei-hilink-status ]; then
        echo "git pull $usrdir/huawei-hilink-status"
        cd $usrdir/huawei-hilink-status
        git branch
        git status
        git pull
        echo
    fi

    if [ -d $usrdir/oem_openHab ]; then
        echo "git pull $usrdir/oem_openHab"
        cd $usrdir/oem_openHab
        git branch
        git status
        git pull
        echo
    fi

    if [ -d $usrdir/oem_node-red ]; then
        echo "git pull $usrdir/oem_node-red"
        cd $usrdir/oem_node-red
        git branch
        git status
        git pull
        echo
    fi
fi

# -----------------------------------------------------------------

if [ "$type" == "all" ] || [ "$type" == "firmware" ]; then

    if [ "$firmware" == "emonpi" ]; then
        $usrdir/emonpi/update/emonpi.sh
    fi

    if [ "$firmware" == "rfm69pi" ]; then
        $usrdir/emonpi/update/rfm69pi.sh
    fi
    
    if [ "$firmware" == "rfm12pi" ]; then
        $usrdir/emonpi/update/rfm12pi.sh
    fi
fi

# -----------------------------------------------------------------

if [ "$type" == "all" ] || [ "$type" == "emonhub" ]; then
    echo "Start emonhub update script:"
    # Run emonHub update script to update emonhub.conf nodes
    $usrdir/emonpi/update/emonhub.sh
    echo
fi

# -----------------------------------------------------------------

if [ "$type" == "all" ] || [ "$type" == "emoncms" ]; then    
    echo "Start emoncms update:"
    # Run emoncms update script to pull in latest emoncms & emonhub updates
    $usrdir/emonpi/update/emoncms.sh $emonSD_pi_env
    echo
fi

# -----------------------------------------------------------------

if [ "$type" == "all" ] && [ "$emonSD_pi_env" = "1" ]; then
    echo
    # Wait for update to finish
    echo "Starting emonPi LCD service.."
    sleep 5
    sudo service emonPiLCD restart
    echo

    if [ -f /usr/bin/rpi-ro ]; then
        rpi-ro
    fi
fi

# -----------------------------------------------------------------

datestr=$(date)

echo
echo "-------------------------------------------------------------"
echo "emonPi update done: $datestr" # this text string is used by service runner to stop the log window polling, DO NOT CHANGE!
echo "-------------------------------------------------------------"

# -----------------------------------------------------------------

if [ "$type" == "all" ] || [ "$type" == "emoncms" ]; then
    echo "restarting service-runner"
    # old service runner
    killall service-runner
    # new service runner
    sudo systemctl restart service-runner.service 
fi

