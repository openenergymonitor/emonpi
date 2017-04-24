#!/usr/bin/env bash

# 1) Check to see if ethernet is UP and get ip address
log=$(tail /var/log/syslog -n 100 | grep -E "wpa_supplicant|dhcpcd|avahi-daemon")
echo "$log"
