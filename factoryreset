#!/bin/bash
# Run sudo su
# ./factoryreset

echo "emonPi Factory reset script"

if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root: sudo su ./factoryreset"
   exit 1
fi

rpi-rw

image_version=$(ls /boot | grep emonSD)
echo "$image_version"
image_date=${image_version:0:16}
# Check first 14 characters of filename
if [[ "$image_date" == "emonSD-17Jun2015" ]]
then
  image="old"
  echo "$image image"
else
  image="new"
  echo "$image image"
fi

if [[ "${image_version:0:6}" == "emonSD" ]]
then
    echo "Image version: $image_version"
else
    echo "Non OpenEnergyMonitor offical emonSD image, no gurantees this script will work :-/"
    read -p "Press any key to continue...or CTRL+C to exit " -n1 -s
fi

ans=n
read -p "Shutdown when complete (y/n)? Else rebooting in 10s " -t 10 ans

VERSION=$(sed 's/\..*//' /etc/debian_version)

# if version is greater than 8 then we must be running Stretch or newer
if [ "$VERSION"  -gt "8" ]; then
        VERSION="stretch"
        echo "Detected $VERSION or newer"
else
        VERSION="jessie"
        echo "$VERSION or older distro detected"
fi


echo "Ensure tracking latest Emoncms & emonpi stable branch"
cd /home/pi/emonpi
git status
git checkout master
git reset --hard HEAD
git pull
git status

cd /var/www/emoncms
git status
git checkout stable
git reset --hard HEAD
git pull
git status

echo
sudo service emonhub stop
sudo service feedwriter stop
sudo service mqtt_input stop
sudo service redis-server stop

echo "remove old conf & backup files"
sudo rm /home/pi/data/*.conf
sudo rm /home/pi/data/*.sql

echo "emoncms.conf reset"
touch /home/pi/data/emoncms.conf
sudo chown pi:www-data /home/pi/data/emoncms.conf
sudo chmod 664 /home/pi/data/emoncms.conf

echo "emonpi emoncms restore default settings"
cp /var/www/emoncms/default.emonpi.settings.php /var/www/emoncms/settings.php

if [[ $image == "old" ]]
then    # Legacy image use emonhub.conf without MQTT authenitication
   echo "Start with fresh config: copy LEGACY default emonhub.conf"
   echo "/home/pi/emonhub/conf/old.default.emonhub.conf /home/pi/data/emonhub.conf"
   cp /home/pi/emonhub/conf/old.default.emonhub.conf /home/pi/data/emonhub.conf

else    # Newer Feb15+ image use latest emonhub.conf with MQTT node variable topic structure and MQTT authentication enabled
   echo "Start with fresh config: copy NEW default emonpi emonhub.conf"
   echo "cp /home/pi/emonhub/conf/emonpi.default.emonhub.conf /home/pi/data/emonhub.conf"
   cp /home/pi/emonhub/conf/emonpi.default.emonhub.conf /home/pi/data/emonhub.conf
 
   echo "Resetting emonPi pi user password to emonpi2016"
passwd pi<<EOF
emonpi2016
emonpi2016
EOF
fi
sudo chown pi:www-data /home/pi/data/emonhub.conf
sudo chmod 666 /home/pi/data/emonhub.conf

echo "deleting phpfina and phptimeseries data"
sudo rm -rf /home/pi/data/phpfina
sudo rm -rf /home/pi/data/phptimeseries

echo "creating new phpfina and phptimeseries folders"
sudo mkdir /home/pi/data/phpfina
sudo mkdir /home/pi/data/phptimeseries
sudo chown www-data:root /home/pi/data/phpfina
sudo chown www-data:root /home/pi/data/phptimeseries

echo "deleting mysql emoncms database"
if [[ $image == "old" ]]
then
  mysql -u root -e "drop database emoncms" -praspberry
  echo "creating new mysql emoncms database"
  mysql -u root -e "create database emoncms" -praspberry
else
  mysql -u root -e "drop database emoncms" -pemonpimysql2016
  echo "creating new mysql emoncms database"
  mysql -u root -e "create database emoncms" -pemonpimysql2016
fi



echo "Delete logs & backup data"
sudo rm /home/pi/data/*.log
sudo rm /home/pi/data/*.gz
sudo rm /home/pi/data/uploads/*.gz
sudo rm /home/pi/data/import -rf

echo "clear WiFi config /etc/wpa_supplicant/wpa_supplicant.conf"
echo "ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev" > /etc/wpa_supplicant/wpa_supplicant.conf
echo "update_config=1" >> /etc/wpa_supplicant/wpa_supplicant.conf
echo "country=GB" >> /etc/wpa_supplicant/wpa_supplicant.conf
sudo chmod 776 /etc/wpa_supplicant/wpa_supplicant.conf


if [ -f /home/pi/data/dhcpd.leases ]; then
   echo "Removing wifiAP dhcpd.leases"
   sudo rm /home/pi/data/dhcpd.leases
   sudo rm /home/pi/data/dhcpd.leases~
fi

echo "remove git user credentials"
sudo rm /home/pi/.gitconfig

echo "Set default emonpi default git credentials" 
git config --global user.email "pi@emonpi.com"
git config --global user.name "emonPi"

# Dont touch NodeRED this is an Emoncms factory reset 
#echo "restore nodered flow"
#cp /home/pi/oem_node-red/flows_emonpi.json /home/pi/data/node-red/flows_emonpi.json 

echo "Clear bash history"
history -c
>~/.bash_history
rm /home/pi/.bash_history

echo "Clear and re-generate SSH keys"
sudo rm /home/pi/.ssh/*
sudo rm /etc/ssh/ssh_host_*
sudo dpkg-reconfigure openssh-server

echo "Clean up packages"
sudo apt-get clean

if [ "$VERSION" = "stretch" ]; then
   echo "Disabling SSH"
   sudo update-rc.d ssh disable
   sudo invoke-rc.d ssh stop
   sudo rm /boot/ssh
fi

echo
if [[ "$ans" == "" ]] ; then ans=n;fi
if [[ "$ans" == "y" ]] ;then
echo "Shutting down Raspberry Pi";
sudo halt;
else
echo "REBOOTING Raspberry Pi";
sudo init 6
fi

exit 0
sudo rm /etc/ssh/ssh_host_*
sudo dpkg-reconfigure openssh-server

echo "Clean up packages"
sudo apt-get clean

if [ "$VERSION" = "stretch" ]; then
   echo "Disabling SSH"
   sudo update-rc.d ssh disable
   sudo invoke-rc.d ssh stop
   sudo rm /boot/ssh
fi

echo
if [[ "$ans" == "" ]] ; then ans=n;fi
if [[ "$ans" == "y" ]] ;then
echo "Shutting down Raspberry Pi";
sudo halt;
else
echo "REBOOTING Raspberry Pi";
sudo init 6
fi

exit 0sudo rm /etc/ssh/ssh_host_*
sudo dpkg-reconfigure openssh-server

echo "Clean up packages"
sudo apt-get clean

if [ "$VERSION" = "stretch" ]; then
   echo "Disabling SSH"
   sudo update-rc.d ssh disable
   sudo invoke-rc.d ssh stop
   sudo rm /boot/ssh
fi

echo
if [[ "$ans" == "" ]] ; then ans=n;fi
if [[ "$ans" == "y" ]] ;then
echo "Shutting down Raspberry Pi";
sudo halt;
else
echo "REBOOTING Raspberry Pi";
sudo init 6
fi

exit 0
