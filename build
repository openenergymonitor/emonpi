#!/bin/bash

echo "Set hostname to emonpi"
sudo sed -i 's/raspberrypi/emonpi/' /etc/hosts
sudo sed -i 's/raspberrypi/emonpi/' /etc/hostname

echo "disable serial console"
sudo wget https://raw.github.com/lurch/rpi-serial-console/master/rpi-serial-console -O /usr/bin/rpi-serial-console && sudo chmod +x /usr/bin/rpi-serial-console
sudo rpi-serial-console disable

echo "enable serial uploads with avrdue and autoreset"
git clone https://github.com/openenergymonitor/avrdude-rpi.git ~/avrdude-rpi && ~/avrdude-rpi/install

git clone https://github.com/openenergymonitor/emonpi.git ~/emonpi  

echo "install shutdown service"
sudo ~/shutdownpi/install

echo "install LCD"
sudo ~/emonpi/lcd/install



