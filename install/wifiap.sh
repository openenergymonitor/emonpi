#!/bin/bash
source config.ini

# Based on https://github.com/openenergymonitor/emonpi/tree/master/wifiAP

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

# -------------------------------------------------------------

echo "Configuring /etc/default/hostapd"
sudo sed -i 's~^#DAEMON_CONF=.*~DAEMON_CONF="/etc/hostapd/hostapd.conf"~' /etc/default/hostapd

if grep -Fq "lease-file-name" /etc/dhcp/dhcpd.conf; then
    echo "dhcpd.conf configuration already exists"
else
    echo "Appending dhcpd.conf configuration"
    conf=$(cat $usrdir/emonpi/wifiAP/append-dhcpd.conf)
    echo "$conf" | sudo tee -a /etc/dhcp/dhcpd.conf
fi

# -------------------------------------------------------------

echo "Applying modifications to dhcpd.conf"

option='option domain-name "example.org";'
sudo sed -i "s~^$option~#$option~" /etc/dhcp/dhcpd.conf

option='option domain-name-servers ns1.example.org, ns2.example.org;'
sudo sed -i "s~^$option~#$option~" /etc/dhcp/dhcpd.conf

option='authoritative;'
sudo sed -i "s~^#$option~$option~" /etc/dhcp/dhcpd.conf

# -------------------------------------------------------------

echo "Creating: "$usrdir/data/dhcpd.leases
sudo touch $usrdir/data/dhcpd.leases

# -------------------------------------------------------------

echo "Applying modifications to dhcpd.conf"

option='DHCPDv4_CONF=/etc/dhcp/dhcpd.conf'
sudo sed -i "s~^#$option~$option~" /etc/default/isc-dhcp-server

option='DHCPDv4_PID=/var/run/dhcpd.pid'
sudo sed -i "s~^#$option~$option~" /etc/default/isc-dhcp-server

sudo sed -i 's~INTERFACESv4=""~INTERFACESv4="wlan0"~' /etc/default/isc-dhcp-server

option='INTERFACESv4="wlan0"'
sudo sed -i "s~^#$option~$option~" /etc/default/isc-dhcp-server

# Setup NAT forwarding

# To bridge eth0 to wlan0 while still allowing us to access the Pi via
# SSH (pure network bridge does not)

# echo "Applying modifications to sysctl.conf"
# option='net.ipv4.ip_forward=1'
# sudo sed -i "s~^#$option~$option~" /etc/sysctl.conf

# Second, to enable NAT in the kernel, run the following commands to bridge eth0 the wlan0:
# sudo iptables -t nat -A POSTROUTING -o eth0 -j MASQUERADE
# sudo iptables -A FORWARD -i eth0 -o wlan0 -m state --state RELATED,ESTABLISHED -j ACCEPT
# sudo iptables -A FORWARD -i wlan0 -o eth0 -j ACCEPT

# If bridge is also required to eth1 (for 3G dongle) also add:
# sudo iptables -t nat -A POSTROUTING -o eth1 -j MASQUERADE
# sudo iptables -A FORWARD -i eth1 -o wlan0 -m state --state RELATED,ESTABLISHED -j ACCEPT
# sudo iptables -A FORWARD -i wlan0 -o eth1 -j ACCEPT

# Save the iptables rules to /etc/iptables.ipv4.nat
# sudo sh -c "iptables-save > /etc/iptables.ipv4.nat"

# Now edit the file /etc/network/interfaces and add the following line to the bottom of the file:
# up iptables-restore < /etc/iptables.ipv4.nat

# Add /etc/rc.local:
# /opt/emon/emonpi/wifiAP/startAP.sh

sudo ln -s $usrdir/emonpi/wifiAP/wifiAP.sh /usr/local/sbin/wifiAP

sudo systemctl unmask hostapd
sudo systemctl enable hostapd

echo "-------------------------------------------------------------"
