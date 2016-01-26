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
4. Serial port (/dev/ttyAMA0) 
5. emonPi LCD service 
5. emonHub
6. Mosquitto MQTT server with authentication
7. Emoncms V9 Core (stable branch)
3. Low write mode Emoncms optimisations
8. Emoncms install & configure modules: node, app, dashboards, wifi
9. Emoncms MQTT service
10. emonPi update script
10. Open UFW ports
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




# 3. Serial port (/dev/ttyAMA0) & cmdline.txt exit

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


# 7. Install Emoncms V9 Core (stable branch)

Follow [Emoncms Raspbian Jessie install guide](https://github.com/emoncms/emoncms/blob/master/docs/RaspberryPi/readme.md)

## emonPi specific settings

* MYSQL root password: `emonpimysql2016`
* MYSQL `emoncms` user password: `emonpiemoncmsmysql2016`
* Use emonPi default settings: 
	* `cd /var/www/emoncms && cp default.emonpi.settings.php settings.php`
* Create feed directories in RW partition `/home/pi/data` instead of `/var/lib`: 
	* `sudo mkdir /home/pi/data/{phpfiwa,phpfina,phptimeseries}`
	* `sudo chown www-data:root /home/pi/data/{phpfiwa,phpfina,phptimeseries}`


# 8. Low write mode Emoncms optimisations

[Related forum discussion thread](http://openenergymonitor.org/emon/node/11695)

Follow [Raspberry Pi Emoncms Low-Write guide](https://github.com/emoncms/emoncms/blob/master/docs/RaspberryPi/Low-write-mode.md) from [Setting up logging on read-only FS](https://github.com/emoncms/emoncms/blob/master/docs/RaspberryPi/Low-write-mode.md#setup-logfile-environment) onwards (we have earlier setup read-only mode):

* [Setting up logging on read-only FS](https://github.com/emoncms/emoncms/blob/master/docs/RaspberryPi/Low-write-mode.md#setup-logfile-environment)
* [Move PHP sessions to tmpfs (RAM)](https://github.com/emoncms/emoncms/blob/master/docs/RaspberryPi/Low-write-mode.md#move-php-sessions-to-tmpfs-ram)
* [Configure Redis](https://github.com/emoncms/emoncms/blob/master/docs/RaspberryPi/Low-write-mode.md#configure-redis)


No need to [Enable Low-write mode in emoncms](https://github.com/emoncms/emoncms/blob/master/docs/RaspberryPi/Low-write-mode.md#enable-low-write-mode-in-emoncms) since these chnages to `settings.php` are already set in `default.emonpi.settings.php` that we copied accross earlier. 

# 9. Emoncms install & configure modules: node, app, dashboards, wifi

```
cd /var/www/emoncms/Modules
git clone https://github.com/emoncms/dashboard.git
git clone https://github.com/emoncms/app.git
git clone https://github.com/emoncms/wifi.git
git clone https://github.com/emoncms/nodes.git
```
After installing modules check and apply database updates in Emoncms Admin. 

## Configure WiFi Module 

Follow install instructions in [WiFi module Readme](https://github.com/emoncms/wifi/blob/9.0/README.md) to give web user permission to execute system WLAN commands. 

## Install wifi-check script

Add wifi-check script to /user/local/bin

	sudo ln -s /home/pi/emonpi/wifi-check /usr/local/bin/wifi-check

Make wifi check script run as root crontab every 5min:

	sudo crontab -e 

Add the line:

	*/5 * * * * /usr/local/bin/wifi-check > /var/log/wificheck.log 2>&1" mycron ; 

## Setup Nodes Module 

Follow setup Readme in [Nodes Module repo](https://github.com/emoncms/nodes) to install `emoncms-nodes-service script`. 





# emonPi Backup / Import

[Forum Topic](http://openenergymonitor.org/emon/node/11843)

# open ports 

https://github.com/emoncms/emoncms/blob/low-write/docs/install.md#security