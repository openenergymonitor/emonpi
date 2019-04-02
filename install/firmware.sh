#! /bin/sh
source config.ini

# --------------------------------------------------------------------------------
# Enable serial uploads with avrdude and autoreset
# --------------------------------------------------------------------------------
cd $usrdir
git clone https://github.com/openenergymonitor/avrdude-rpi.git ~/avrdude-rpi && ~/avrdude-rpi/install

cd $usrdir
if ![ -d $usrdir/RFM2Pi ]; then
    git clone https://github.com/openenergymonitor/RFM2Pi
fi
