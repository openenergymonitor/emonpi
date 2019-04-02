#!/bin/bash

service=$1

if [ -f /etc/init.d/$service ]; then
  echo "removing initd $service service"
  sudo /etc/init.d/$service stop
  sudo rm /etc/init.d/$service
fi

if [ -L /lib/systemd/system/$service.service ] || [ -f /lib/systemd/system/$service.service ]; then
  echo "Removing $service.service"
  sudo systemctl stop $service.service
  sudo systemctl disable $service.service
  sudo rm /lib/systemd/system/$service.service
  sudo systemctl daemon-reload
fi

# if the service still exists in /etc then remove it
if [ -L /etc/systemd/system/$service.service ] || [ -f /etc/systemd/system/$service.service ]; then
  echo "Removing $service.service from /etc/systemd/system"
  sudo rm /etc/systemd/system/$service.service
fi
