#!/usr/bin/env bash

# 1) Check to see if ethernet is UP and get ip address
eth0status=$(cat /sys/class/net/eth0/carrier)
wlan0status=$(cat /sys/class/net/wlan0/carrier)

if [ -f /sys/class/net/ap0/carrier ]
then
    ap0status=$(cat /sys/class/net/ap0/carrier)
else
    ap0status=0
fi

now=$(date)
echo "$now"

if [[ $ap0status -eq 1 ]]
then
    echo "AP Already active"
elif [[ $eth0status -eq 1 ]]
then
    echo "Ethernet eth0 up" 
    eth0ip=$(ip addr show eth0 | grep -Po 'inet \K[\d.]+')
    echo "IP Address: $eth0ip"
elif [[ $wlanstatus -eq 1 ]]
then
    echo "WIFI wlan0 up" 
    wlan0ip=$(ip addr show wlan0 | grep -Po 'inet \K[\d.]+')
    echo "IP Address: $wlan0ip"
else
    if grep -q "network" /etc/wpa_supplicant/wpa_supplicant-wlan0.conf
    then
       echo "WiFi network config found"
    else
       echo "WiFi not configured"
       echo "Going for WIFI AP startup"
       sleep 5
       systemctl start wpa_supplicant@ap0.service
       echo "WiFi AP started"
    fi
fi
