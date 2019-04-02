#!/bin/bash
source config.ini

# --------------------------------------------------------------------------------
# Enable serial uploads with avrdude and autoreset
# --------------------------------------------------------------------------------
cd $usrdir
if [ ! -d $usrdir/avrdude-rpi ]; then
    git clone https://github.com/openenergymonitor/avrdude-rpi.git
    $usrdir/avrdude-rpi/install
else 
    echo "- avrdude-rpi already exists"
fi

cd $usrdir
if [ ! -d $usrdir/RFM2Pi ]; then
    git clone https://github.com/openenergymonitor/RFM2Pi
else
    echo "- RFM2Pi already exists"
fi
