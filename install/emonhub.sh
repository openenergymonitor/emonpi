#!/bin/bash
echo "-------------------------------------------------------------"
echo "emonHub install"
echo "-------------------------------------------------------------"
emonSD_pi_env=$1

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
usrdir=${DIR/\/emonpi\/install/}

sudo apt-get install -y python-serial python-configobj
sudo pip install paho-mqtt requests

if [ "$emonSD_pi_env" = "1" ]; then
    # RaspberryPi Serial configuration
    # disable Pi3 Bluetooth and restore UART0/ttyAMA0 over GPIOs 14 & 15;
    # Review should this be: dtoverlay=pi3-miniuart-bt?
    sudo sed -i -n '/dtoverlay=pi3-disable-bt/!p;$a dtoverlay=pi3-disable-bt' /boot/config.txt

    # We also need to stop the Bluetooth modem trying to use UART
    sudo systemctl disable hciuart

    # Remove console from /boot/cmdline.txt
    sudo sed -i "s/console=serial0,115200 //" /boot/cmdline.txt

    # stop and disable serial service??
    sudo systemctl stop serial-getty@ttyAMA0.service
    sudo systemctl disable serial-getty@ttyAMA0.service
fi

cd $usrdir
if [ -d $usrdir/data ]; then
    mkdir data
fi

git clone https://github.com/openenergymonitor/emonhub.git

cd $usrdir/emonhub
sudo ./install.systemd
sudo systemctl start emonhub.service

# Temporary: replace with update to default settings file
sed -i "s/loglevel = DEBUG/loglevel = WARNING/" /$usrdir/data/emonhub.conf
