#!/bin/bash

echo "-------------------------------------------------------------"
echo "Main Update Script"
echo "-------------------------------------------------------------"

# -----------------------------------------------------------------
# Check environment
# -----------------------------------------------------------------

username=$1
type=$2
firmware=$3
homedir="/home/$username"
datestr=$(date)

echo "Date:" $datestr
echo "EUID: $EUID"
echo "username: $username"
echo "homedir: $homedir"
echo "type: $type"
echo "firmware: $firmware"

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
if [ $pi ] && [ -d $homedir ]; then
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
    
    for image_name in "emonSD-07Nov16" "emonSD-03May16" "emonSD-26Oct17" "emonSD-13Jun18" "emonSD-30Oct18"; do
        if [ "$image_version" == "$image_name" ]; then
            echo "emonSD base image check passed...continue update"
        else
            echo "ERROR: emonSD base image old or undefined...update will not continue"
            echo "See latest verson: https://github.com/openenergymonitor/emonpi/wiki/emonSD-pre-built-SD-card-Download-&-Change-Log"
            echo "Stopping update"
            exit 0
        fi 
    done
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

if [ "$emonSD_pi_env" = "1" ]; then
    # Check if we have an emonpi LCD connected, 
    # if we do assume EmonPi hardware else assume RFM69Pi
    lcd27=$(sudo $homedir/emonpi/lcd/emonPiLCD_detect.sh 27 1)
    lcd3f=$(sudo $homedir/emonpi/lcd/emonPiLCD_detect.sh 3f 1)

    if [ $lcd27 == 'True' ] || [ $lcd3f == 'True' ]; then
        hardware="EmonPi"
    else
        hardware="rfm2pi"
    fi
    echo "Hardware detected: $hardware"
    
    # Stop emonPi LCD servcice
    # sudo service emonPiLCD stop

    # Display update message on LCD
    # sudo $homedir/emonpi/lcd/./emonPiLCD_update.py
fi

# sudo apt-get update

# Ensure rpi gpio is latest version and gpiozero is installed, required for V2.2.0 LCD script
# rng-tools used to speed up entropy generation https://community.openenergymonitor.org/t/cant-connect-to-local-emoncms/9498/7
# sudo apt-get install python-gpiozero python-rpi.gpio rng-tools -y
# sudo pip install paho-mqtt --upgrade

# -----------------------------------------------------------------

if [ "$type" == "all" ]; then

    if [ -d $homedir/RFM2Pi ]; then
        echo "git pull $homedir/RFM2Pi"
        cd $homedir/RFM2Pi
        git branch
        git status
        git pull
        echo
    fi

    if [ -d $homedir/usefulscripts ]; then
        echo "git pull $homedir/usefulscripts"
        cd $homedir/usefulscripts
        git branch
        git status
        git pull
        echo
    fi

    if [ -d $homedir/huawei-hilink-status ]; then
        echo "git pull $homedir/huawei-hilink-status"
        cd $homedir/huawei-hilink-status
        git branch
        git status
        git pull
        echo
    fi

    if [ -d $homedir/oem_openHab ]; then
        echo "git pull $homedir/oem_openHab"
        cd $homedir/oem_openHab
        git branch
        git status
        git pull
        echo
    fi

    if [ -d $homedir/oem_node-red ]; then
        echo "git pull $homedir/oem_node-red"
        cd $homedir/oem_node-red
        git branch
        git status
        git pull
        echo
    fi
fi

# -----------------------------------------------------------------
# Firmware update
# -----------------------------------------------------------------

if [ "$type" = "firmware" ]; then

    if [ "$firmware" == "emonpi" ]; then
      $homedir/emonpi/update/emonpi.sh
    fi

    if [ "$firmware" == "rfm69pi" ]; then
      $homedir/emonpi/update/rfm69pi.sh
    fi
    
    if [ "$firmware" == "rfm12pi" ]; then
      $homedir/emonpi/update/rfm12pi.sh
    fi
fi

# -----------------------------------------------------------------

if [ "$type" != "firmware" ]; then
    echo
    echo "Start emonhub update script:"
    # Run emonHub update script to update emonhub.conf nodes
    $homedir/emonpi/update/emonhub.sh $homedir
    echo

    echo "Start emoncms update:"
    # Run emoncms update script to pull in latest emoncms & emonhub updates
    $homedir/emonpi/update/emoncms.sh $homedir $emonSD_pi_env $emoncms_dir
    echo

    if [ "$emonSD_pi_env" = "1" ]; then
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
fi

datestr=$(date)
echo
echo "-------------------------------------------------------------"
echo "Update done: $datestr" # this text string is used by service runner to stop the log window polling, DO NOT CHANGE!
echo "-------------------------------------------------------------"
echo
if [ "$type" != "firmware" ]; then
    echo "restarting service-runner"
    # old service runner
    killall service-runner
    # new service runner
    sudo systemctl restart service-runner.service 
fi
