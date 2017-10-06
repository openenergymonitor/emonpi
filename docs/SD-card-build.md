# emonSD (emonPi & emonBase) SD card build

Glyn Hudson - January 2016

This guide replaces the imagebuild.md (renamed to old.imagebuild.md) and emonPi install.sh script which has never really worked reliably (renamed to old.install.sh). The guide cites the [Emoncms Raspberry Pi install guides](https://github.com/emoncms/emoncms/tree/master/docs/RaspberryPi) exhaustively compiled by Paul Reed.

[**User docs & setup guide**](http://guide.openenergymonitor.org)

[**Pre-built image repository & changelog**](https://github.com/openenergymonitor/emonpi/wiki/emonSD-pre-built-SD-card-Download-&-Change-Log)

***

# Features  
- Base image **RASPBIAN JESSIE LITE 2015-11-21** (Kernel 4.1)
- 4GB SD card size, partitions can be expanded if required (8GB SD card shipped with emonPi)

1. Initial setup
2. Root filesystem read-only with RW data partition (~/data)
4. Serial port (/dev/ttyAMA0)
5. Memory Setup
5. emonPi LCD service
5. emonHub
6. Mosquitto MQTT server with authentication
7. Emoncms V9 Core (stable branch)
3. Low write mode Emoncms optimisations
8. Install & configure Emoncms modules
10. Install emonPi update & import / export (emonPi backup) script
9. Install Emoncms MQTT input service
10. LightWave RF MQTT OKK transmitter
10. Open UFW ports
12. openHab
12. nodeRED
13. 3G / GSM HiLink Huawei USB modem dongle support
14. Wifi AP mode
15. Language Support
16. Symlink scripts 



# 1. Initial Setup

## Update and install core packages

	sudo apt-get update -y
	sudo apt-get upgrade -y
	cd /home/pi && sudo apt-get install git-core -y
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

## Custom emonPi MOTD (message of the day)

Use custom motd to alert users they are logging into an emonPi with RW / RO toggle instructions:

```
sudo rm /etc/motd
sudo ln -s /home/pi/emonpi/motd /etc/motd
```


# 3. Serial port setup 


To allow the emonPi to communicate with the RasPi via serial we need to disconnect the terminal console from /tty/AMA0.

## Use custom cmdline.txt

	sudo mv /boot/cmdline.txt /boot/original.cmdline.txt
	sudo cp /home/pi/emonpi/cmdline.txt /boot/

This changes the  a custom cmdline.txt file:

From:

	dwc_otg.lpm_enable=0 console=ttyAMA0,115200 console=tty1 root=/dev/mmcblk0p2 rootfstype=ext4 elevator=deadline fsck.repair=yes rootwait

To:

	dwc_otg.lpm_enable=0 console=tty1 elevator=noop root=/dev/mmcblk0p2 rootfstype=ext4 fsck.repair=yes rootwait

Note changing `elevator=deadline` to `elevator=noop` disk scheduler. Noop that is best recommend for flash disks, this will result in a reduction in disk I/O performance

"Noop: Inserts all the incoming I/O requests to a First In First Out queue and implements request merging. Best used with storage devices that does not depend on mechanical movement to access data (yes, like our flash drives). Advantage here is that flash drives does not require reordering of multiple I/O requests unlike in normal hard drives" - [Full article](http://androidmodguide.blogspot.co.uk/p/io-schedulers.html). [Forum topic discussion](http://openenergymonitor.org/emon/node/11695).

## Disable serial console boot

 	sudo systemctl stop serial-getty@ttyAMA0.service
 	sudo systemctl disable serial-getty@ttyAMA0.service

## Enable serial uploads with avrdude and autoreset

	git clone https://github.com/openenergymonitor/avrdude-rpi.git ~/avrdude-rpi && ~/avrdude-rpi/install

## Raspberry Pi 3 Compatibility 

The emonPi communicates with the RasPi via GPIO 14/15 which on the Model B,B+ and Pi2 is mapped to UART0. However on the Pi3 these pins are mapped to UART1 since UART0 is now used for the bluetooth module. However UART1 is software UART and baud rate is dependant to clock speed which can change with the CPU load, undervoltage and temperature; therefore not stable enough. One hack is to force the CPU to a lower speed ( add `core_freq=250` to `/boot/cmdline.txt`)which cripples the Pi3 performace. A better solution for the emonPi is to disable BT and map UART1 back to UART0 (ttyAMA0) so we can talk to the emomPi in the same way as before.  

Update to pull in latest firmware and device tree (DT) 
```
sudo apt-get update
sudo apt-get upgrade
sudo apt-get dist-upgrade
sudo rpi-update
```

To disable Pi3 Bluetooth and restore UART0/ttyAMA0 over GPIOs 14 & 15 modify:

	sudo nano /boot/config.txt
	
Add to the end of the file

	dtoverlay=pi3-disable-bt

We also need to run to stop BT modem trying to use UART

	sudo systemctl disable hciuart

See [RasPi device tree commit](https://github.com/raspberrypi/firmware/commit/845eb064cb52af00f2ea33c0c9c54136f664a3e4) for `pi3-disable-bt` and [forum thread discussion](https://www.raspberrypi.org/forums/viewtopic.php?f=107&t=138223)
	

Reboot then test serial comms with:

	sudo minicom -D /dev/ttyAMA0 -b38400
	
You should see data from emonPi ATmega328, sending serial `v` should result in emonPi returning it's firmware version and config settings. 

To fix SSHD bug (when using the on board WiFi adapter and NO ethernet). [Forum thread](https://openenergymonitor.org/emon/node/12566). Edit ` /etc/ssh/sshd_config ` and append:

	IPQoS cs0 cs0

# Memory Setup

Append `gpu_mem=16` to `/boot/config.txt` this caps the RAM available to the GPU. Since we are running headless this will give us more RAM at the expence of the GPI

# 4. emonPi LCD service

Enable I2C module, install packages needed for emonPi LCD service `python-smbus i2c-tools python-rpi.gpio python-pip redis-server` and `pip install uptime redis paho-mqtt`, and install emonPi LCD python scrip service to run at boot

	sudo emonpi/lcd/./install

Restart and test if I2C LCD is detected, it should be on address `0x27`:

	sudo i2cdetect -y 1




# 5. Install mosquitto MQTT

## Install & configure Mosquitto MQTT server, PHP MQTT
```
wget http://repo.mosquitto.org/debian/mosquitto-repo.gpg.key
sudo apt-key add mosquitto-repo.gpg.key
cd /etc/apt/sources.list.d/
sudo wget http://repo.mosquitto.org/debian/mosquitto-jessie.list
sudo apt-get update
sudo apt-get install mosquitto mosquitto-clients libmosquitto-dev -y
sudo pecl install Mosquitto-alpha
(​Hit enter to autodetect libmosquitto location)
```

If PHP extension config files `/etc/php5/cli/conf.d/20-mosquitto.ini` and `/etc/php5/apache2/conf.d/20-mosquitto.ini` don't exist then create with:

    sudo sh -c 'echo "extension=mosquitto.so" > /etc/php5/cli/conf.d/20-mosquitto.ini'
    sudo sh -c 'echo "extension=mosquitto.so" > /etc/php5/apache2/conf.d/20-mosquitto.ini'

Turn off Mosquitto persistence and enable authentication:

	sudo nano /etc/mosquitto/mosquitto.conf

Set `persistence false` and add the lines:

	allow_anonymous false
	password_file /etc/mosquitto/passwd

Create a password file for MQTT user `emonpi` with:

	sudo mosquitto_passwd -c /etc/mosquitto/passwd emonpi

Enter password `emonpimqtt2016` (default)

**Test MQTT**

Open *another shell window* to subscribe to a test topic:

	mosquitto_sub -v -u 'emonpi' -P 'emonpimqtt2016' -t 'test/topic'

 In the first shell oublish to the test topic :
 
	mosquitto_pub -u 'emonpi' -P 'emonpimqtt2016' -t 'test/topic' -m 'helloWorld'
	
If all is working we should see `helloWorld` in the second shell

##  Install  emonHub

Install other EmonHub dependencies if they have not been installed already by emonPi LCD service
```
sudo apt-get install -y python-pip python-serial python-configobj”
sudo pip install paho-mqtt pydispatcher
```

## Install emonHub (emon-pi) variant:

	cd ~/
	git clone https://github.com/openenergymonitor/emonhub.git && emonhub/install


Check log file:

	tail /var/log/emonhub/emonhub.log

We can check data is being posted to MQTT by subscribing to the base topic `emon/#` if new node variable MQTT topic structure is used or `emonhub/#` if old MQTT CSV topic structure is used

	mosquitto_sub -v -u 'emonpi' -P 'emonpimqtt2016' -t 'emon/#'

# 7. Install Emoncms V9 Core (stable branch)
=======

Follow [Emoncms Raspberry Pi Raspbian Jessie install guide](https://github.com/emoncms/emoncms/blob/master/docs/RaspberryPi/readme.md)

## emonPi specific settings

* MYSQL root password: `emonpimysql2016`
* MYSQL `emoncms` user password: `emonpiemoncmsmysql2016`
* Use emonPi default settings:
	* `cd /var/www/emoncms && cp default.emonpi.settings.php settings.php`
* Create feed directories in RW partition `/home/pi/data` instead of `/var/lib`:
	* `sudo mkdir /home/pi/data/{phpfina,phptimeseries}`
	* `sudo chown www-data:root /home/pi/data/{phpfina,phptimeseries}`

Note: at time of writing the version of `php5-redis` included in the Raspbian Jessie sources (2.2.5-1) caused Apache to crash (segmentation errrors in Apache error log). Installing the latest stable version (2.2.7) of php5-redis from github fixed the issue. This step probably won't be required in the future when the updated version of php5-redis makes it's way into the sources. The check the version in the sources: `sudo apt-cache show php5-redis`

```
git clone --branch 2.2.7 https://github.com/phpredis/phpredis
cd phpredis
(check the version we are about to install:)
​cat php_redis.h | grep VERSION
phpize
./configure 
sudo make 
sudo make install
```


## Move MYSQL database location

The MYSQL database for Emoncms is usually in `/var/lib`, since the emonPi runs the root FS as read-only we need to move the MYSQL database to the RW partition `home/pi/data/mysql`. Follow stepsL [Moving MYSQL database in Emoncms Read-only documentation](https://github.com/emoncms/emoncms/blob/master/docs/RaspberryPi/read-only.md#move-mysql-database)


# 8. Low write mode Emoncms optimisations

[Related forum discussion thread](http://openenergymonitor.org/emon/node/11695)

Follow [Raspberry Pi Emoncms Low-Write guide](https://github.com/emoncms/emoncms/blob/master/docs/RaspberryPi/Low-write-mode.md) from [Setting up logging on read-only FS](https://github.com/emoncms/emoncms/blob/master/docs/RaspberryPi/Low-write-mode.md#setup-logfile-environment) onwards (we have earlier setup read-only mode):

* [Setting up logging on read-only FS](https://github.com/emoncms/emoncms/blob/master/docs/RaspberryPi/Low-write-mode.md#setup-logfile-environment)
	* After running install we want to use emonPi specific rc.local instead:
		* `sudo rm /etc/rc.local`
		* `sudo ln -s /home/pi/emonpi/rc.local_jessieminimal /etc/rc.local`

* [Move PHP sessions to tmpfs (RAM)](https://github.com/emoncms/emoncms/blob/master/docs/RaspberryPi/Low-write-mode.md#move-php-sessions-to-tmpfs-ram)
* [Configure Redis](https://github.com/emoncms/emoncms/blob/master/docs/RaspberryPi/Low-write-mode.md#configure-redis)
	* After redis config **Ensure all redis databases have been removed from `/var/lib/redis`**
* Disable apache access log:
	* `sudo nano /etc/apache2/conf-enabled/other-vhosts-access-log.conf`
	* comment out the access log
* Ensure apache error log goes to /var/log/apache2/error.log
 	*  `sudo nano /etc/apache2/envvars`
 	*  Ensure `export APACHE_LOG_DIR=/var/log/apache2/$SUFFIX`

* [Reduce garbage in syslog due to Raspbian bug](https://openenergymonitor.org/emon/node/12566):

	* Edit the `/etc/rsyslog.conf` and comment out the last 4 lines that use xconsole:

```
#daemon.*;mail.*;\

#       news.err;\

#       *.=debug;*.=info;\

#       *.=notice;*.=warn       |/dev/xconsole
```

No need to [Enable Low-write mode in emoncms](https://github.com/emoncms/emoncms/blob/master/docs/RaspberryPi/Low-write-mode.md#enable-low-write-mode-in-emoncms) since these changes to `settings.php` are already set in `default.emonpi.settings.php` that we installed earlier.

# 9. Emoncms install & configure modules: node, app, dashboards, wifi

```
cd /var/www/emoncms/Modules
git clone https://github.com/emoncms/dashboard.git
git clone https://github.com/emoncms/app.git
git clone https://github.com/emoncms/wifi.git
git clone https://github.com/emoncms/config
git clone https://github.com/emoncms/graph

cd /home/pi/
git clone https://github.com/emoncms/backup
```
~~git clone https://github.com/emoncms/nodes.git~~ Deprecated in favour of Emoncms PHP MQTT input see `Install Emoncms MQTT input service`

After installing modules check and apply database updates in Emoncms Admin.

## Configure WiFi Module

Follow install instructions in [WiFi module Readme](https://github.com/emoncms/wifi/blob/9.0/README.md) to give web user permission to execute system WLAN commands.

Move `wpa_supplicant.conf` (file where WiFi network authentication details are stored) to RW partition with symlink back to /etc:

	sudo mv /etc/wpa_supplicant/wpa_supplicant.conf /home/pi/data/wpa_supplicant.conf
	sudo ln -s /home/pi/data/wpa_supplicant.conf /etc/wpa_supplicant/wpa_supplicant.conf  

## Install wifi-check script

To check if wifi is connected every 5min and re-initiate connection if not.

Add wifi-check script to `/user/local/bin`:

	sudo ln -s /home/pi/emonpi/wifi-check /usr/local/bin/wifi-check

Make wifi check script run as root crontab every 5min:

	sudo crontab -e

Add the line:

``*/5 * * * * /usr/local/bin/wifi-check > /var/log/wificheck.log 2>&1" mycron ; ``

## Setup emonPi Backup & import module

Follow [instructions on emonPi backup module page](https://github.com/emoncms/backup) to symlink the Emoncms module from the folder in /home/pi/backup that also contains the backup shell scripts

~~## Setup Nodes Module~~

~~Follow setup Readme in [Nodes Module repo](https://github.com/emoncms/nodes) to install `emoncms-nodes-service script`.~~  Deprecated in favour of Emoncms PHP MQTT input see `Install Emoncms MQTT input service`



# 10. Install emonPi update

Add Pi user cron entry:

	crontab -e

Add the cron entries to check if emonpi update or emonpi backup has been triggered once every 60s:

```
MAILTO=""

# # Run emonPi update script ever min, scrip exits unless update flag exists in /tmp
* * * * * /home/pi/emonpi/service-runner >> /var/log/service-runner.log 2>&1
```

To enable triggering update on first factory boot (when emonpiupdate.log does not exist) add entry to `rc.local`:

	/home/pi/emonpi/./firstbootupdate

This line should be present already if the emonPi speicifc `rc.local` file has been symlinked into place, if not:

	sudo rm /etc/rc.local
	sudo ln -s /home/pi/emonpi/emonpi/rc.local_jessieminimal /etc/rc.local

The `update` script looks for a flag in `/tmp/emonpiupdate` which is set when use clicks Update in Emoncms. If flag is present then the update script runs `emonpiupdate`, `emoncmsupdate` and `emonhubupdate` and logs to `~/data/emonpiupdate.log`. If log file is not present then update is ran on first (factory) boot.

# 11. Install Emoncms MQTT input service

To enable data posted to base topic `emon/` topic to appear in Emoncms inputs e.g `emon/emontx/power1 10` creates an input from emonTx node with power1 with value 10.

[Follow install guide]( https://github.com/emoncms/emoncms/blob/master/docs/RaspberryPi/MQTT.md)

	sudo service mqtt_input start

[Forum Thread discussing emonhub MQTT support](http://openenergymonitor.org/emon/node/12091)

*Previously EmonHub posted decoded data to MQTT topic with CSV format to  `emonhub/rx/5/values 10,11,12` which the Nodes module suscribed to and decoded. Using the MQTT input service in conjunction with the new EmonHub MQTT node topic structure depreciates the Nodes module*

# 12. Lightwave RF MQTT service

[Follow LightWave RF MQTT GitHub Repo Install Guide](https://github.com/openenergymonitor/lightwaverf-pi)

# 13 Open required ports & Enable Firewall

Apache web server `sudo ufw allow 80/tcp` and`sudo ufw allow 443/tcp`

SSH server: `sudo ufw allow 22/tcp`

Mosquitto MQTT: `sudo ufw allow 1883/tcp`

Redis: `sudo ufw allow 6379/tcp`

OpenHAB: `sudo ufw allow 8080/tcp`

NodeRed: `sudo ufw allow 1880/tcp`

	sudo ufw enable

# 14. Install NodeRED

[Follow OEM NodeRED install guide](https://github.com/openenergymonitor/oem_node-red) with OEM examples.

Default flows admin user: `emonpi` and password `emonpi2016`

**Need to make default emonPi flow using new MQTT**

# 15. Install openHAB

[Follow OEM openHAB install guide](https://github.com/openenergymonitor/oem_openHab) with OEM examples

# 16. 3G / GSM HiLink Huawei USB modem dongle support

[Follow Huawei Hi-Link RasPi setup guide](https://github.com/openenergymonitor/huawei-hilink-status/blob/master/README.md) to setup HiLink devices and useful status utility. The emonPiLCD now uses the same Huawei API to display GSM / 3G connection status and signal level on the LCD. 

# 17. Wifi AP mode

Wifi Access Point mode is useful when using emonPi without a interent connection of with a 3G dongle.

[Follow guide to install hostpad and DHCP](https://github.com/openenergymonitor/emonpi/blob/master/docs/wifiAP.md)

Including installing start/ stop script to start the AP and also brdige the 3G dongle interface on eth1 to wlan0

# 18. Language Support 

```
sudo apt-get install gettext
sudo dpkg-reconfigure locales
```
Select required languages from the list by presseing [Space], hit [Enter to install], see [language translations supported by Emoncms](https://github.com/emoncms/emoncms/tree/master/Modules/user/locale)

`sudo apache2 restart`

[more info on gettext](https://github.com/emoncms/emoncms/blob/master/docs/gettext.md)

# 19 Symlink scripts 

### emonSDexpand:

`sudo ln -s /home/pi/usefulscripts/sdpart/sdpart_imagefile /sbin/emonSDexpand`

### Fstab: 

`sudo ln -s /home/pi/emonpi/fstab /etc/fstab`
