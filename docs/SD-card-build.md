# emonPi SD card build

Glyn Hudson - January 2016 

This guide replaces the imagebuild.md (renamed to old.imagebuild.md) and emonPi install.sh script which has never really worked reliably (renamed to old.install.sh). The guide cites the [Emoncms Raspberry Pi install guides](https://github.com/emoncms/emoncms/tree/master/docs/RaspberryPi) exhaustively compiled by Paul Reed.

Forum discussion: 
- [Dec 22nd image beta based on Minibianpi](http://openenergymonitor.org/emon/node/11799)

# Features  
- Base image RASPBIAN JESSIE LITE 2015-11-21 (Kernel 4.1)
- 4GB SD card size, partitions can be expanded if required (8GB SD card shipped with emonPi)

1. Initial setup
2. Root filesystem read-only with RW data partition (~/data)
3. Low write mode & optimisations
4. emonPi LCD service & serial port (/dev/ttyAMA0)
5. emonHub
6. Mosquitto MQTT server with authentication
7. Emoncms V9 Core (stable branch)
8. Emoncms install & configure modules: node, app, dashboards, wifi
8. Emoncms logger & logrotate
9. Emoncms MQTT service
10. Emoncms export / import module
10. LightWave RF MQTT OKK transmitter
12. openHab
12. nodeRED



# 1. Initial Setup

## Update and install core packages 

	sudo apt-get update -y
	sudo apt-get upgrade -y
	sudo apt-get install git-core -y
	git clone https://github.com/openenergymonitor/emonpi

## Change password, set international options & expand Filesystem

Using raspi-config utility:

	sudo raspi-config

Raspbian minimal image is 2GB by default, expand to fill 4GB. Then reboot

## Change hostname

From 'raspberry' to 'emonpi'

	sudo nano /etc/hosts 
	sudo nano /etc/hostname

## Change Password 

From 'raspberrypi' to 'emonpi2016'

	sudo passwd

# 2. Root filesystem read-only with RW data partition (~/data)


Follow Raspberry Pi Emoncms Read-Only guide: 

[https://github.com/emoncms/emoncms/blob/master/docs/RaspberryPi/read-only.md](https://github.com/emoncms/emoncms/blob/master/docs/RaspberryPi/read-only.md)



# 3. Low write mode & optimisations

[Related forum discussion thread](http://openenergymonitor.org/emon/node/11695)

Follow Raspberry Pi Emoncms Low-Write guide: 

[https://github.com/emoncms/emoncms/blob/master/docs/RaspberryPi/Low-write-mode.md](https://github.com/emoncms/emoncms/blob/master/docs/RaspberryPi/Low-write-mode.md)