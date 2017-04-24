#!/usr/bin/env bash

# 1) Check to see if ethernet is UP and get ip address
eth0status=$(cat /sys/class/net/eth0/carrier)
wlan0status=$(cat /sys/class/net/wlan0/carrier)

if [[ $eth0status -eq 1 ]]
then
    echo "Ethernet eth0 up" 
    eth0ip=$(/sbin/ifconfig eth0 | grep 'inet addr' | cut -d: -f2 | awk '{print $1}')
    echo "IP Address: $eth0ip"
elif [[ $wlanstatus -eq 1 ]]
then
    echo "WIFI wlan0 up" 
    wlan0ip=$(/sbin/ifconfig wlan0 | grep 'inet addr' | cut -d: -f2 | awk '{print $1}')
    echo "IP Address: $wlan0ip"
else
    wificonfig=$(cat /home/pi/data/wpa_supplicant.conf)
    emptywificonfig=$(cat /home/pi/emonpi/wifiAP/wpa_supplicant_check.conf)

    if [ "$wificonfig" == "$emptywificonfig" ]; then
        echo "Blank WIFI config detected"
        echo "Going for WIFI AP startup"
        touch /home/pi/data/wifiAP-enabled
        wifiAP start
    fi
fi
