#!/usr/bin/env bash

if [ -f /home/pi/data/wifiAP-enabled ]; then
    rm /home/pi/data/wifiAP-enabled
fi
if [ -f /opt/openenergymonitor/data/wifiAP-enabled ]; then
    rm /opt/openenergymonitor/data/wifiAP-enabled
fi

wifiAP stop
sudo reboot
