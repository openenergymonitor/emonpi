#!/bin/bash

# emonPi update for use with service-runner add following entry to crontab:
# * * * * * /home/pi/emonpi/service-runner >> /var/log/service-runner.log 2>&1
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
usrdir=${DIR/\/emonpi/}

if [ -z "$2" ]; then
    type="all"
    firmware=$1
else
    type=$1
    firmware=$2
fi

# Clear log update file
cat /dev/null > $usrdir/data/emonpiupdate.log

echo "Starting update via service-runner-update.sh (v2.0) >"

# make file system read-write
if [ -f /usr/bin/rpi-rw ]; then
  rpi-rw
fi

# -----------------------------------------------------------------

# Check emonSD version
image_version=$(ls /boot | grep emonSD)

if [ "$image_version" = "" ]; then
    echo "- Could not find emonSD version file"
    emonSD_pi_env=0
else 
    echo "- emonSD version: $image_version"
    valid=0
    
    save_to_update=$(curl -s https://raw.githubusercontent.com/openenergymonitor/emonpi/master/safe-update)
    while read -r image_name; do
        if [ "$image_version" == "$image_name" ]; then
            echo "emonSD base image check passed...continue update"
            valid=1
        fi
    done <<< "$save_to_update"

    if [ $valid == 0 ]; then
        echo "ERROR: emonSD base image old or undefined...update will not continue"
        echo "See latest verson: https://github.com/openenergymonitor/emonpi/wiki/emonSD-pre-built-SD-card-Download-&-Change-Log"
        echo "Stopping update"
        exit 0
    fi
fi

# -----------------------------------------------------------------

# Pull in latest emonpi repo before then running updated update scripts
echo "git pull $usrdir/emonpi"
cd $usrdir/emonpi
sudo rm -rf hardware/emonpi/emonpi2c/
git branch
git status
git pull

# Run update in main update script
$usrdir/emonpi/update/main.sh $type $firmware
