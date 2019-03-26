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

echo "Starting update via service-runner-update.sh (v1.1.1) >"

# make file system read-write
if [ -f /usr/bin/rpi-rw ]; then
  rpi-rw
fi

# Pull in latest emonpi repo before then running updated update scripts
echo "git pull $usrdir/emonpi"
cd $usrdir/emonpi
sudo rm -rf hardware/emonpi/emonpi2c/
git branch
git status
git pull

# Run update in main update script
$usrdir/emonpi/update/main.sh $type $firmware
