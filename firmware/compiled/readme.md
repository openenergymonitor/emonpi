# Update or upload steps

Update Atmega328 firmware on RasPi to latest.hex. In order of user friendliness:

## 1. Run emonPi update from local Emoncms

In local emoncms e.g. [http://emonpi/emoncms](http://emonpi/emoncms)

`Setup > Admin > emonPi Update`

## 2. Run update bash script

        sudo service emonhub stop
	sudo .update

## 3. Run avrdude manually

	sudo service emonhub stop

	avrdude -v -c arduino -p ATMEGA328P -P /dev/ttyAMA0 -b 115200 -U flash:w:latest.hex

	sudo service emonhub start
