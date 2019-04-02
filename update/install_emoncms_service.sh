#!/bin/bash

servicepath=$1
service=$2

if [ -d /etc/systemd ] && [ -f $servicepath ]; then
  if [ -f /etc/init.d/$service ]; then
    echo "removing initd $service service"
    sudo /etc/init.d/$service stop
    sudo rm /etc/init.d/$service
  fi
  
  # service will be symlinked to /etc/systemd/system by "systemctl enable"
  if [ -f /etc/systemd/system/$service.service ]; then
    if ! [ -L /etc/systemd/system/$service.service ]; then
      echo "Removing hard copy of $service.service from /etc/systemd/system (should be symlink)"
      sudo systemctl stop $service.service
      sudo systemctl disable $service.service
      sudo rm /etc/systemd/system/$service.service
      sudo systemctl daemon-reload
    fi
  fi
  
  if [ -f /lib/systemd/system/$service.service ]; then
    if ! [ -L /lib/systemd/system/$service.service ]; then
      echo "Removing hard copy of $service.service in /lib/systemd/system (should be symlink)"
      sudo systemctl stop $service.service
      sudo systemctl disable $service.service
      sudo rm /lib/systemd/system/$service.service
      sudo systemctl daemon-reload
    fi
  fi
  
  if [ ! -f /lib/systemd/system/$service.service ]; then
    echo "Installing $service.service in /lib/systemd/system (creating symlink)"
    sudo ln -s $servicepath /lib/systemd/system
    sudo systemctl enable $service.service
    sudo systemctl start $service.service
  else 
    echo "$service.service already installed"
  fi
fi
