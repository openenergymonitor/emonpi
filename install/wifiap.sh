#!/bin/bash
source config.ini

echo "-------------------------------------------------------------"

# Install dhcp server and Hostapd
sudo apt-get install -y isc-dhcp-server hostapd

echo "-------------------------------------------------------------"

echo "Checking hostapd version:"
hostapd -v

echo "-------------------------------------------------------------"

if [ ! -f /etc/hostapd/hostapd.conf ]; then
    echo "Installing hostapd.conf"
    sudo cp $usrdir/emonpi/wifiAP/hostapd.conf /etc/hostapd/hostapd.conf
    
    # Manual start: you should see a Wifi network called emonPi and emonpi2016 raspberry
    # sudo hostapd -dd /etc/hostapd/hostapd.conf
else
    echo "hostapd.conf already exists"
fi 

echo "-------------------------------------------------------------"

echo "Configuring /etc/default/hostapd"
sudo sed -i 's~^#DAEMON_CONF=.*~DAEMON_CONF="/etc/hostapd/hostapd.conf"~' /etc/default/hostapd

echo "-------------------------------------------------------------"

if grep -Fq "lease-file-name" /etc/dhcp/dhcpd.conf; then
    echo "dhcpd.conf configuration already exists"
else
    echo "Appending dhcpd.conf configuration"
    conf=$(cat $usrdir/emonpi/wifiAP/append-dhcpd.conf)
    echo "$conf" | sudo tee -a /etc/dhcp/dhcpd.conf
fi
echo "-------------------------------------------------------------"
