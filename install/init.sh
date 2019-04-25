#!/bin/bash

user=$USER
usrdir=/opt/emon

sudo apt-get update -y
sudo apt-get install -y git-core

sudo mkdir $usrdir
sudo chown $user $usrdir
cd $usrdir

git clone https://github.com/openenergymonitor/emonpi.git

cd $usrdir/emonpi/install
./main.sh
cd

rm init.sh
