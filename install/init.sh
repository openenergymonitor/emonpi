#!/bin/bash

user=pi

sudo apt-get update -y
sudo apt-get install -y git-core

sudo mkdir /usr/emoncms
sudo chown $user /usr/emoncms
cd /usr/emoncms

git clone https://github.com/openenergymonitor/emonpi.git

/usr/emoncms/emonpi/install/main.sh

cd

rm init.sh
