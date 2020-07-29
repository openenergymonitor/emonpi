#!/usr/bin/env bash

# 1) Check to see if ethernet is UP and get ip address
log=$(tail /var/log/syslog -n 1000 | grep -E "wpa_supplicant|dhcpcd|avahi-daemon" | tail -n 20)
echo "$log"
