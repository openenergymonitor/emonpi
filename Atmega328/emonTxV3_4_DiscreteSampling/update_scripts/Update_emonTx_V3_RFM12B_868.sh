# !/bin/bash


echo "starting avrdude upload emonTx V3.4 RFM12B 868Mhz"
avrdude  -u -c arduino -p ATMEGA328P -P /dev/ttyUSB0 -b 115200 -U flash:w:/home/pi/emonTxFirmware/emonTxV3/RFM/emonTxV3.4/emonTxV3_4_DiscreteSampling/compiled/emonTxV3_RFM12B_latest_868.hex
