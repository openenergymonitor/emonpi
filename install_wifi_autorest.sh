#!/bin/bash
cd /opt/scripts/tools/
./update_kernel.sh
reboot
cd ~
ntpdate -b -s -u pool.ntp.org
apt-get update && apt-get install git
git clone https://github.com/adafruit/wifi-reset.git
cd wifi-reset
chmod +x install.sh
./install.sh

cd /home/debian/emonpi/
cc usbreset.c -o usbreset
chmod +x usbreset
chmod +x usb_reset.sh
