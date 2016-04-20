# !/bin/bash


echo "starting avrdude upload emonTx V3.4 RFM69CW 433Mhz"
avrdude  -u -c arduino -p ATMEGA328P -P /dev/ttyUSB0 -b 115200 -U flash:w:/home/pi/emonTxFirmware/emonTxV3/RFM/emonTxV3.4/emonTxV3_4_DiscreteSampling/compiled/emonTxV3_RFM69CW_latest_433.hex
