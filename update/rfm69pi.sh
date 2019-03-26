#!/bin/bash
echo "-------------------------------------------------------------"
echo "RFM69Pi Firmware Update"
echo "-------------------------------------------------------------"
homedir=$1

echo "Getting latest RFM69Pi release info from github"
download_url="$(curl -s https://api.github.com/repos/openenergymonitor/RFM2Pi/releases | grep browser_download_url | head -n 1 | cut -d '"' -f 4)"
version="$(curl -s https://api.github.com/repos/openenergymonitor/RFM2Pi/releases | grep tag_name | head -n 1 |  cut -d '"' -f 4)"
echo "Latest RFM69Pi firmware: V"$version

echo "downloading latest RFM69Pi firmware from github releases:"
echo $download_url
echo "Saving to $homepi/data/firmware/rfm69pi-"$version".hex"

if [ ! -d $homepi/data/firmware ]; then
  mkdir $homepi/data/firmware
fi

wget -q $download_url -O $homepi/data/firmware/rfm69pi-$version.hex

if [ -f $homepi/data/firmware/rfm69pi-$version.hex ]; then
  sudo service emonhub stop
  echo
  echo "Flashing RFM69Pi with V" $version
  echo
  avrdude -v -c arduino -p ATMEGA328P -P /dev/ttyAMA0 -b 38400 -U flash:w:$homedir/data/firmware/rfm69pi-$version.hex
  sudo service emonhub start
  echo "Flashing RFM69Pi with V" $version " done"
else
 echo "Firmware download failed...check network connection"
fi
