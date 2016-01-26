# emonPi SD card build

Glyn Hudson - January 2016 

# IN DEVELOPMENT

This guide replaces the imagebuild.md (renamed to old.imagebuild.md) and emonPi install.sh script which has never really worked reliably (renamed to old.install.sh). The guide cites the [Emoncms Raspberry Pi install guides](https://github.com/emoncms/emoncms/tree/master/docs/RaspberryPi) exhaustively compiled by Paul Reed.

Forum discussion: 
- [Dec 22nd image beta based on Minibianpi (old beta, latest image is based on Raspbian Jessie Lite)](http://openenergymonitor.org/emon/node/11799)

# Features  
- Base image RASPBIAN JESSIE LITE 2015-11-21 (Kernel 4.1)
- 4GB SD card size, partitions can be expanded if required (8GB SD card shipped with emonPi)

1. Initial setup
2. Root filesystem read-only with RW data partition (~/data)
4. Serial port (/dev/ttyAMA0) 
5. emonPi LCD service 
5. emonHub
6. Mosquitto MQTT server with authentication
7. Emoncms V9 Core (stable branch)
8. Emoncms install & configure modules: node, app, dashboards, wifi
8. Emoncms logger & logrotate
3. Low write mode optimisations
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
    git clone https://github.com/openenergymonitor/RFM2Pi

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


[Follow Raspberry Pi Emoncms Read-Only guide](https://github.com/emoncms/emoncms/blob/master/docs/RaspberryPi/read-only.md)




# 3. Serial port setup (cmdline.txt edit)

To allow the emonPi to communicate with the RasPi via serial we need to disconnect the terminal console from /tty/AMA0. 

## Use custom cmdline.txt

	sudo mv /boot/cmdline.txt /boot/original.cmdline.txt
	sudo cp /home/pi/emonpi/cmdline.txt /boot/

This changes the  a custom cmdline.txt file:

From:
	
	dwc_otg.lpm_enable=0 console=ttyAMA0,115200 console=tty1 root=/dev/mmcblk0p2 rootfstype=ext4 elevator=deadline fsck.repair=yes rootwait

To:
	
	wc_otg.lpm_enable=0 console=tty1 elevator=noop root=/dev/mmcblk0p2 rootfstype=ext4 fsck.repair=yes rootwait

Note changing `elevator=deadline` to `elevator=noop` disk scheduler. Noop that is best recommend for flash disks, this will result in a reduction in disk I/O performance 

"Noop: Inserts all the incoming I/O requests to a First In First Out queue and implements request merging. Best used with storage devices that does not depend on mechanical movement to access data (yes, like our flash drives). Advantage here is that flash drives does not require reordering of multiple I/O requests unlike in normal hard drives" - [Full article](http://androidmodguide.blogspot.co.uk/p/io-schedulers.html). [Forum topic discussion](http://openenergymonitor.org/emon/node/11695). 

## Enable serial uploads with avrdude and autoreset

	git clone https://github.com/openenergymonitor/avrdude-rpi.git ~/avrdude-rpi && ~/avrdude-rpi/install
	
Test serial comms with:

	sudo minicom -D /dev/ttyAMA0 -b38400

# 4. emonPi LCD service

Enable I2C module, install packages needed for emonPi LCD service `python-smbus i2c-tools python-rpi.gpio python-pip redis-server` and `pip install uptime redis paho-mqtt`, and install emonPi LCD python scrip service to run at boot

	sudo emonpi/lcd/./install

Restart and test if I2C LCD is detected, it should be on address `0x27`: 

	sudo i2cdetect -y 1

# 5. Install mosquitto MQTT & emonHub 

## Install & configure mosquitto mqtt service (required for emoHub)
```
wget http://repo.mosquitto.org/debian/mosquitto-repo.gpg.key
sudo apt-key add mosquitto-repo.gpg.key
cd /etc/apt/sources.list.d/
sudo wget http://repo.mosquitto.org/debian/mosquitto-jessie.list
sudo apt-get update
sudo apt-get install mosquitto mosquitto-clients -y
```

Turn off persistence and enable authentication:

	sudo nano /etc/mosquitto/mosquitto.conf

Set `persistence false` and add the lines:

	allow_anonymous false
	password_file /etc/mosquitto/passwd

Create a password file for MQTT user `emonpi` with:

	sudo mosquitto_passwd -c /etc/mosquitto/passwd emonpi

Enter password `emonpimqtt2016` (default)

Test by publishing to a topic:
 
	mosquitto_pub -u 'emonpi' -P 'emonpimqtt2016' -t 'test/topic' -m 'helloWorld'

While in another shell window subscribe to that topic, if all is working we should see `helloWord` 

	mosquitto_sub -v -u 'emonpi' -P 'emonpimqtt2016' -t 'test/topic'

Install other EmonHub dependencies if they have not been installed already by emonPi LCD service
```
sudo apt-get install python-pip
sudo pip install paho-mqtt
sudo pip install pydispatcher
```

## Install OpenEnergyMonitor (emon-pi) emonHub variant:

	git clone https://github.com/openenergymonitor/emonhub.git && emonhub/install
	sudo service emonhub start

The emon-pi variant of emonHub locates the config file in the RW partition `/home/pi/data/emonhub.conf`, this config file includes the default emonpi MQTT authentication details. 

Check log file:

	tail /var/log/emonhub/emonhub.log 

Successful log output shows data being received from emonPi (node 5) and posted to MQTT:

```
pi@emonpi:~ $ cat /var/log/emonhub/emonhub.log 
2016-01-26 10:54:58,325 INFO     MainThread EmonHub emonHub 'emon-pi' variant v1.1
2016-01-26 10:54:58,326 INFO     MainThread Opening hub...
2016-01-26 10:54:58,327 INFO     MainThread Logging level set to DEBUG
2016-01-26 10:54:58,328 INFO     MainThread Creating EmonHubJeeInterfacer 'RFM2Pi' 
2016-01-26 10:54:58,332 DEBUG    MainThread Opening serial port: /dev/ttyAMA0 @ 38400 bits/s
2016-01-26 10:55:00,338 INFO     MainThread RFM2Pi device firmware version & configuration: not available
2016-01-26 10:55:00,340 INFO     MainThread Setting RFM2Pi frequency: 433 (4b)
2016-01-26 10:55:01,342 INFO     MainThread Setting RFM2Pi group: 210 (210g)
2016-01-26 10:55:02,344 INFO     MainThread Setting RFM2Pi quiet: 0 (0q)
2016-01-26 10:55:03,350 INFO     MainThread Setting RFM2Pi baseid: 5 (5i)
2016-01-26 10:55:04,352 INFO     MainThread Setting RFM2Pi calibration: 230V (1p)
2016-01-26 10:55:05,355 DEBUG    MainThread Setting RFM2Pi subchannels: ['ToRFM12']
2016-01-26 10:55:05,356 DEBUG    MainThread Interfacer: Subscribed to channel' : ToRFM12
2016-01-26 10:55:05,357 DEBUG    MainThread Setting RFM2Pi pubchannels: ['ToEmonCMS']
2016-01-26 10:55:05,358 DEBUG    MainThread Interfacer: Subscribed to channel' : ToRFM12
2016-01-26 10:55:05,360 INFO     MainThread Creating EmonHubMqttInterfacer 'MQTT' 
2016-01-26 10:55:05,364 INFO     MainThread MQTT Init mqtt_host=127.0.0.1 mqtt_port=1883 mqtt_user=emonpi
2016-01-26 10:55:05,366 DEBUG    RFM2Pi     device settings updated: E i5 g210 @ 433 MHz USA 0
2016-01-26 10:55:05,372 DEBUG    MainThread MQTT Subscribed to channel' : ToEmonCMS
2016-01-26 10:55:05,374 INFO     MainThread Creating EmonHubEmoncmsHTTPInterfacer 'emoncmsorg' 
2016-01-26 10:55:05,376 DEBUG    MainThread emoncmsorg Subscribed to channel' : ToEmonCMS
2016-01-26 10:55:05,474 INFO     MQTT       Connecting to MQTT Server
2016-01-26 10:55:05,473 DEBUG    RFM2Pi     1 NEW FRAME : OK 5 0 0 0 0 0 0 216 89 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 (-0)
2016-01-26 10:55:05,480 INFO     MQTT       connection status: Connection successful
```
We can check data is being posted to MQTT by subscribing to the base topic `emonhub/#`

	mosquitto_sub -v -u 'emonpi' -P 'emonpimqtt2016' -t 'emonhub/#'













# Low write mode & optimisations

[Related forum discussion thread](http://openenergymonitor.org/emon/node/11695)



Since we have enabled read-only the first part of the [Raspberry Pi Emoncms Low-Write guide](https://github.com/emoncms/emoncms/blob/master/docs/RaspberryPi/Low-write-mode.md) does not apply we can dive stright into the second parts from   
[Setting up logging on read-only FS](https://github.com/emoncms/emoncms/blob/master/docs/RaspberryPi/Low-write-mode.md#setup-logfile-environment) onwards 
