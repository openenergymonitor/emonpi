#!/bin/bash

download_url = curl -s https://api.github.com/repos/openenergymonitor/RFM2Pi/releases | grep browser_download_url | head -n 1 | cut -d '"' -f 4
version = curl -s https://api.github.com/repos/openenergymonitor/RFM2Pi/releases | grep tag_name | head -n 1 |  cut -d '"' -f 4

wget $download_url /home/pi/data
filename = basename $download_url

echo
echo "================================="
echo "RFM69Pi update started"
echo "================================="
echo
echo "Flashing RFM69Pi with V" $version
echo


avrdude -v -c arduino -p ATMEGA328P -P /dev/ttyAMA0 -b 38400 -U flash:w:/home/pi/data/$filename

