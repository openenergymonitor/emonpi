#!/usr/bin/env bash

# Script to start / stop WiFi AP and bridge eth1
# https://github.com/openenergymonitor/emonpi/blob/master/docs/wifiAP.md
# Only works with BCM43143 e.g. RasPi3 / offical RasPi dongle
	
if [ $EUID -ne 0 ]; then
   echo "This script must be run as root"
   exit 1
fi


if [ -z "$1" ]; then
    echo "Please specify start or stop as argument"
    exit 1
fi


# Put emonPi file system into RW mode
rpi-rw

if [ "$1" = "start" ]; then
	echo "Starting AP.....please wait process could take about 10-20s"
    	echo "Wifi connection will now be lost...wait 30s then connect to SSID 'emonPi' SSID with password 'emonpi2016' then browse to http://192.168.42.1"
	sudo ifdown wlan0
	# sleep 4
	sudo ifconfig wlan0 down
	
	# if eth1 exists and is up then bridge to wlan0
	FOUND=`grep "eth1" /proc/net/dev`
    	if  [ -n "$FOUND" ] ; then
        	echo "eth1 up"
        	echo "Bridge eth1 (GSM dongle) to WiFi AP"
		# Remove bridge routes if exist to avoid duplicates 
        	sudo iptables -t nat -D POSTROUTING -o eth1 -j MASQUERADE >/dev/null 2>&1
        	sudo iptables -D FORWARD -i eth1 -o wlan0 -m state --state RELATED,ESTABLISHED -j ACCEPT >/dev/null 2>&1
        	sudo iptables -D FORWARD -i wlan0 -o eth1 -j ACCEPT >/dev/null 2>&1
		# Add bridge nodes        	
		sudo iptables -t nat -A POSTROUTING -o eth1 -j MASQUERADE
        	sudo iptables -A FORWARD -i eth1 -o wlan0 -m state --state RELATED,ESTABLISHED -j ACCEPT
		sudo iptables -A FORWARD -i wlan0 -o eth1 -j ACCEPT
    	fi
    
    	# sleep 5
	echo "Set static IP addres of emonPi AP 192.168.42.1"
	sudo ifconfig wlan0 192.168.42.1
    	# sleep 5
	# Start DHCP server to offer AP clients DHCP
	echo "Start isc-dhcp-server"

         if [ -f /home/pi/data/dhcpd.leases ]; then
                echo "Removing wifiAP /home/pi/data/dhcpd.leases"
                rm /home/pi/data/dhcpd.leases
                touch /home/pi/data/dhcpd.leases
        else
                touch /home/pi/data/dhcpd.leases
        fi

	if [ -f /var/lib/dhcp/dhcpd.leases ]; then
  		echo "Removing wifiAP /var/lib/dhcp/dhcpd.leases"
   		rm /var/lib/dhcp/dhcpd.leases
		touch /var/lib/dhcp/dhcpd.leases
	else
		touch /var/lib/dhcp/dhcpd.leases
	fi
	sudo service isc-dhcp-server start
	# sleep 5

	# Start AP
	echo "Start emonPi Wifi AP...."
	sudo service hostapd start

fi

if [ "$1" = "stop" ]; then
# Stop Wifi AP and restore normal Managed Client STA mode
        echo "Stop hostapd Wifi AP"
        sudo service hostapd stop
        echo "Stop isc-dhcp-server"
        sudo service isc-dhcp-server stop
        # sleep 5
        sudo kill $(pgrep -f "wpa_supplicant -B")
	sudo ifconfig wlan0 down
	sudo ifconfig wlan0 up
	sudo rm -r /var/run/wpa_supplicant/*
	# sudo wpa_supplicant -B -iwlan0 -f/var/log/wpa_supplicant.log -c/etc/wpa_supplicant/wpa_supplicant.conf
	# sleep 10
	sudo dhclient -v -r wlan0
	# sleep 5
	# sudo dhclient -v wlan0

        # Remove bridge routes
        sudo iptables -t nat -D POSTROUTING -o eth1 -j MASQUERADE >/dev/null 2>&1
        sudo iptables -D FORWARD -i eth1 -o wlan0 -m state --state RELATED,ESTABLISHED -j ACCEPT >/dev/null 2>&1
        sudo iptables -D FORWARD -i wlan0 -o eth1 -j ACCEPT >/dev/null 2>&1

        rm /home/pi/data/wifiAP-enabled
fi
rpi-ro
