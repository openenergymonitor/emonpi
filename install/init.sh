#! /bin/sh

USER=pi

sudo apt-get update -y
sudo apt-get install -y git-core

sudo mkdir /usr/emoncms
sudo chown $USER /usr/emoncms
cd /usr/emoncms

git clone -b update_refactor https://github.com/openenergymonitor/emonpi.git

/usr/emoncms/emonpi/install/emonSD_build_test.sh
