# Update or upload steps

sudo service emonhub stop

avrdude -v -c arduino -p ATMEGA328P -P /dev/ttyAMA0 -b 115200 -U flash:w:/home/pi/emonpi/Atmega328/emonPi_RFM69CW_RF12Demo_ContinuousSampling/compiled/emonPi_RFM69CW_RF12Demo_ContinuousSampling.cpp.hex

sudo service emonhub start
