#/!bin/bash
sleep 1
sudo echo BB-UART4 > /sys/devices/bone_capemgr.9/slots
sleep 2
cd /
cd home/debian/emonpi/lcd
sudo python gprs_signal.py
cd /

sudo pon fona
sleep 1
