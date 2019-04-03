#!/bin/bash
source config.ini

echo "-------------------------------------------------------------"
echo "emonHub install"
echo "-------------------------------------------------------------"

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
    sudo systemctl mask serial-getty@ttyAMA0.service
fi

cd $usrdir
if [ ! -d $usrdir/data ]; then
    mkdir data
fi

if [ ! -d $usrdir/emonhub ]; then
    git clone https://github.com/openenergymonitor/emonhub.git
    # modify service file to point to correct config file location
    sudo sed -i "s~/home/pi/data/emonhub.conf~$usrdir/data/emonhub.conf~g" $usrdir/emonhub/service/emonhub.service
    
    sudo rm /usr/share/emonhub
    sudo ln -s $usrdir/emonhub/src /usr/share/emonhub
    sudo useradd -M -r -G dialout,tty -c "emonHub user" emonhub
else 
    echo "- emonhub repository already installed"
fi

if [ ! -f $usrdir/data/emonhub.conf ]; then
    sudo mv $usrdir/emonhub/conf/emonpi.default.emonhub.conf $usrdir/data/emonhub.conf
fi

# Install service
service="emonhub"
servicepath="$usrdir/emonhub/service/emonhub.service"
$usrdir/emonpi/update/install_emoncms_service.sh $servicepath $service

# Temporary: replace with update to default settings file
sed -i "s/loglevel = DEBUG/loglevel = WARNING/" $usrdir/data/emonhub.conf

# Sudoers entry (review!)
sudo visudo -cf $usrdir/emonpi/sudoers.d/emonhub-sudoers && \
sudo cp $usrdir/emonpi/sudoers.d/emonhub-sudoers /etc/sudoers.d/
sudo chmod 0440 /etc/sudoers.d/emonhub-sudoers
echo "emonhub service control sudoers entry installed"
