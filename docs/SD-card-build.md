# emonSD (emonPi & emonBase) SD card build

**As used to build emonSD-26Oct17**

**For download: See [emonSD releases page (Git wiki)](https://github.com/openenergymonitor/emonpi/wiki/emonSD-pre-built-SD-card-Download-%26-Change-Log)**.

***

## emonPi/emonBase Resources

- [**User docs & setup guide**](http://guide.openenergymonitor.org)
- [**Purchase Pre-built SD card**](http://shop.openenergymonitor.com/emonsd-pre-loaded-raspberry-pi-sd-card/)
- [**Purchase emonPi**](shop.openenergymonitor.com/emonpi-3/)
- [**Pre-built image repository & changelog**](https://github.com/openenergymonitor/emonpi/wiki/emonSD-pre-built-SD-card-Download-&-Change-Log)

***

# emonSD Features  

- Base image RASPBIAN JESSIE LITE
- 8GB min SD card size
- Read-only root file system
- Stable Emoncms Core
- Apache web-server
- MQTT
- NodeRed
- OpenHAB


***

# Contents

<!-- toc -->

- [Initial Linux Setup](#initial-linux-setup)
  * [Update](#update)
  * [Change password, set international options & expand File-system](#change-password-set-international-options--expand-file-system)
  * [Change hostname](#change-hostname)
  * [Change Password](#change-password)
  * [Root filesystem read-only with RW data partition](#root-filesystem-read-only-with-rw-data-partition)
  * [Custom MOTD (message of the day)](#custom-motd-message-of-the-day)
  * [Memory Tweak](#memory-tweak)
- [RasPi Serial port setup](#raspi-serial-port-setup)
  * [Use custom cmdline.txt](#use-custom-cmdlinetxt)
  * [Disable serial console boot](#disable-serial-console-boot)
  * [Enable serial uploads with avrdude and autoreset](#enable-serial-uploads-with-avrdude-and-autoreset)
  * [Raspberry Pi 3 Compatibility](#raspberry-pi-3-compatibility)
- [Setup Read-only filesystem](#setup-read-only-filesystem)
- [Install emonPi Services](#install-emonpi-services)
  * [emonPi LCD service](#emonpi-lcd-service)
  * [Setup emonPi update](#setup-emonpi-update)
  * [Setup NTP Update](#setup-ntp-update)
  * [Fix Random seed](#fix-random-seed)
- [Install mosquitto MQTT](#install-mosquitto-mqtt)
- [emonHub](#emonhub)
  * [Install emonHub (emon-pi) variant:](#install-emonhub-emon-pi-variant)
- [Install Emoncms Core](#install-emoncms-core)
  * [emonPi specific Emoncms settings](#emonpi-specific-emoncms-settings)
  * [Move MYSQL database location](#move-mysql-database-location)
  * [Low write mode Emoncms optimisations](#low-write-mode-emoncms-optimisations)
  * [Install Emoncms Modules](#install-emoncms-modules)
    + [Configure Emoncms WiFi Module](#configure-emoncms-wifi-module)
      - [Install wifi-check script](#install-wifi-check-script)
    + [Setup Emoncms Backup & import module](#setup-emoncms-backup--import-module)
  * [Emoncms Language Support](#emoncms-language-support)
- [Install Emoncms MQTT input service](#install-emoncms-mqtt-input-service)
- [Lightwave RF MQTT service](#lightwave-rf-mqtt-service)
- [Configure Firewall](#configure-firewall)
- [Install NodeRED](#install-nodered)
- [Install openHAB](#install-openhab)
- [GSM HiLink Huawei USB modem dongle support](#gsm-hilink-huawei-usb-modem-dongle-support)
- [emonPi Setup Wifi AP mode](#emonpi-setup-wifi-ap-mode)
- [Symlink scripts](#symlink-scripts)
  * [emonSDexpand:](#emonsdexpand)
  * [Fstab:](#fstab)
- [Clean Up](#clean-up)
  * [Remove unused packages](#remove-unused-packages)
  * [Run Factory reset](#run-factory-reset)

<!-- tocstop -->

***

# Initial Linux Setup

## Update

	sudo apt-get update -y
	sudo apt-get upgrade -y

## Change password, set international options & expand File-system

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

## Root filesystem read-only with RW data partition


## Custom MOTD (message of the day)

Use custom motd to alert users they are logging into an emonPi with RW / RO toggle instructions:

```
sudo rm /etc/motd
sudo ln -s /home/pi/emonpi/motd /etc/motd
```

## Memory Tweak

Append `gpu_mem=16` to `/boot/config.txt` this caps the RAM available to the GPU. Since we are running headless this will give us more RAM at the expense of the GPU


***

# RasPi Serial port setup


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

The emonPi communicates with the RasPi via GPIO 14/15 which on the Model B,B+ and Pi2 is mapped to UART0. However on the Pi3 these pins are mapped to UART1 since UART0 is now used for the bluetooth module. However UART1 is software UART and baud rate is dependant to clock speed which can change with the CPU load, undervoltage and temperature; therefore not stable enough. One hack is to force the CPU to a lower speed ( add `core_freq=250` to `/boot/cmdline.txt`) which cripples the Pi3 performance. A better solution for the emonPi is to swap BT and map UART1 back to UART0 (ttyAMA0) so we can talk to the emomPi in the same way as before.  

RasPi 3 by default has bluetooth (BT) mapped to `/dev/AMA0`. To allow use to use high performace hardware serial (dev/ttyAMA0) and move bluetooth to software serial (/dev/ttyS0) we need to add an overlay to `config.txt`. See `/boot/overlays/README` for more details. See [this post for RasPi3 serial explained](https://spellfoundry.com/2016/05/29/configuring-gpio-serial-port-raspbian-jessie-including-pi-3/).

	sudo nano /boot/config.txt

Add to the end of the file

	dtoverlay=pi3-miniuart-bt

We also need to run to stop BT modem trying to use UART

	sudo systemctl disable hciuart

See [RasPi device tree commit](https://github.com/raspberrypi/firmware/commit/845eb064cb52af00f2ea33c0c9c54136f664a3e4) for `pi3-disable-bt` and [forum thread discussion](https://www.raspberrypi.org/forums/viewtopic.php?f=107&t=138223)


Reboot then test serial comms with:

	sudo minicom -D /dev/ttyAMA0 -b38400

You should see data from emonPi ATmega328, sending serial `v` should result in emonPi returning it's firmware version and config settings.

To fix SSHD bug (when using the on board WiFi adapter and NO Ethernet). [Forum thread](https://openenergymonitor.org/emon/node/12566). Edit ` /etc/ssh/sshd_config ` and append:

	IPQoS cs0 cs0

***

# Setup Read-only filesystem

[Follow Raspberry Pi Read-Only guide](https://github.com/emoncms/emoncms/blob/master/docs/RaspberryPi/read-only.md)

# Install emonPi Services

	sudo apt-get install git-core -y
	cd /home/pi
	git clone https://github.com/openenergymonitor/emonpi
	git clone https://github.com/openenergymonitor/RFM2Pi

## emonPi LCD service

Enable I2C module, install packages needed for emonPi LCD service `python-smbus i2c-tools python-rpi.gpio python-pip redis-server` and `pip install uptime redis paho-mqtt`, and install emonPi LCD python scrip service to run at boot

	sudo emonpi/lcd/./install

Restart and test if I2C LCD is detected, it should be on address `0x27`:

	sudo i2cdetect -y 1

## Setup emonPi update

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

## Setup NTP Update

See [forum thread](https://community.openenergymonitor.org/t/emontx-communication-with-rpi/3659/2)

`sudo crontab -e`

Add

```
# Force NTP update every 1 hour

0 * * * * /home/pi/emonpi/ntp_update.sh >> /var/log/ntp_update.log 2>&1
```

## Fix Random seed

Random seed is important for secure ssh and https connections. Using a RO root FS we need to symlink randomised to RW data partition to enable to service to run and be random. See [forum thread](https://community.openenergymonitor.org/t/random-seed/3637).

```
$ rpi-rw
$ cd /var/lib/systemd/
$ sudo mv random-seed ~/data/
$ sudo ln -s ~/data/random-seed
$ rpi-ro
$ sudo systemctl restart systemd-random-seed
$ sudo systemctl status systemd-random-seed
```

Random seed service should now be running.




***

# Install mosquitto MQTT

Install & configure Mosquitto MQTT server, PHP MQTT
```
wget http://repo.mosquitto.org/debian/mosquitto-repo.gpg.key
sudo apt-key add mosquitto-repo.gpg.key
cd /etc/apt/sources.list.d/
sudo wget http://repo.mosquitto.org/debian/mosquitto-jessie.list
sudo apt-get update
sudo apt-get install mosquitto mosquitto-clients libmosquitto-dev -y
sudo pecl install Mosquitto-beta
(​Hit enter to autodetect libmosquitto location)
```

To check which version of Mosquitto pecl has installed run `$ pecl list-upgrades`. Currently emonSD (Oct17) is running Mosquitto 0.3. See this [forum post](https://community.openenergymonitor.org/t/raspbian-stretch/5096/60?u=glyn.hudson) and [this one](https://community.openenergymonitor.org/t/upgrading-emonsd-php-mosquito-version/6265/13) discussing the choice of Mosquitto alpa.

```
pecl list-upgrades
Channel pear.php.net: No upgrades available
Channel pear.swiftmailer.org: No upgrades available
pecl.php.net Available Upgrades (stable):
=========================================
Channel      Package   Local          Remote         Size
pecl.php.net dio       0.0.6 (beta)   0.1.0 (beta)   37kB
pecl.php.net Mosquitto 0.3.0 (beta)   0.4.0 (beta)   24kB
pecl.php.net redis     2.2.5 (stable) 3.1.6 (stable) 196kB
```

To upgrade to Mosquitto 0.4 run (currently not fully tested as of Jan 18): *Update: seems to work fine*

`$ sudo pecl install Mosquitto-0.4.0`

Install PHP Mosquitto extension:

    printf "extension=mosquitto.so" | sudo tee /etc/php5/mods-available/mosquitto.ini 1>&2
    sudo php5enmod mosquitto

Turn off Mosquitto persistence and enable authentication:

	sudo nano /etc/mosquitto/mosquitto.conf

Set `persistence false` and add the lines:

	allow_anonymous false
	password_file /etc/mosquitto/passwd

Set logging to errors only

	log_type error

Create a password file for MQTT user `emonpi` with:

	sudo mosquitto_passwd -c /etc/mosquitto/passwd emonpi

Enter password `emonpimqtt2016` (default)

**Test MQTT**

Open *another shell window* to subscribe to a test topic:

	mosquitto_sub -v -u 'emonpi' -P 'emonpimqtt2016' -t 'test/topic'

 In the first shell oublish to the test topic :

	mosquitto_pub -u 'emonpi' -P 'emonpimqtt2016' -t 'test/topic' -m 'helloWorld'

If all is working we should see `helloWorld` in the second shell

***

# emonHub

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


***

# Install Emoncms Core

*V9 Core (stable branch)*

Follow [Emoncms Raspberry Pi Raspbian Jessie install guide](https://github.com/emoncms/emoncms/blob/master/docs/RaspberryPi/readme.md)

## emonPi specific Emoncms settings

* MYSQL root password: `emonpimysql2016`
* MYSQL `emoncms` user password: `emonpiemoncmsmysql2016`
* Use emonPi default settings:
	* `cd /var/www/emoncms && cp default.emonpi.settings.php settings.php`
* Create feed directories in RW partition `/home/pi/data` instead of `/var/lib`:
	* `sudo mkdir /home/pi/data/{phpfina,phptimeseries}`
	* `sudo chown www-data:root /home/pi/data/{phpfina,phptimeseries}`

*Note: at time of writing the version of `php5-redis` included in the Raspbian Jessie sources (2.2.5-1) caused Apache to crash (segmentation errors in Apache error log). Installing the latest stable version (2.2.7) of php5-redis from github fixed the issue. This step probably won't be required in the future when the updated version of php5-redis makes it's way into the sources. The check the version in the sources: `sudo apt-cache show php5-redis`*

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


## Low write mode Emoncms optimisations

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

## Install Emoncms Modules

```
cd /var/www/emoncms/Modules
git clone https://github.com/emoncms/dashboard.git
git clone https://github.com/emoncms/app.git
git clone https://github.com/emoncms/wifi.git
git clone https://github.com/emoncms/config
git clone https://github.com/emoncms/graph

cd /home/pi/
git clone https://github.com/emoncms/backup
git clone https://github.com/emoncms/postprocess
git clone https://github.com/emoncms/usefulscripts
```

- After installing modules check and apply database updates in Emoncms Admin.

### Configure Emoncms WiFi Module

Follow install instructions in [WiFi module Readme](https://github.com/emoncms/wifi/blob/9.0/README.md) to give web user permission to execute system WLAN commands.

Move `wpa_supplicant.conf` (file where WiFi network authentication details are stored) to RW partition with symlink back to /etc:

	sudo mv /etc/wpa_supplicant/wpa_supplicant.conf /home/pi/data/wpa_supplicant.conf
	sudo ln -s /home/pi/data/wpa_supplicant.conf /etc/wpa_supplicant/wpa_supplicant.conf  

#### Install wifi-check script

To check if wifi is connected every 5min and re-initiate connection if not.

Add wifi-check script to `/user/local/bin`:

	sudo ln -s /home/pi/emonpi/wifi-check /usr/local/bin/wifi-check

Make wifi check script run as root crontab every 5min:

	sudo crontab -e

Add the line:

``*/5 * * * * /usr/local/bin/wifi-check > /var/log/wificheck.log 2>&1" mycron ; ``

### Setup Emoncms Backup & import module

Follow [instructions on emonPi backup module page](https://github.com/emoncms/backup) to symlink the Emoncms module from the folder in /home/pi/backup that also contains the backup shell scripts

## Emoncms Language Support

```
sudo apt-get install gettext
sudo dpkg-reconfigure locales
```
Select required languages from the list by pressing [Space], hit [Enter to install], see [language translations supported by Emoncms](https://github.com/emoncms/emoncms/tree/master/Modules/user/locale)

`sudo apache2 restart`

[more info on gettext](https://github.com/emoncms/emoncms/blob/master/docs/gettext.md)

***

# Install Emoncms MQTT input service

To enable data posted to base topic `emon/` topic to appear in Emoncms inputs e.g `emon/emontx/power1 10` creates an input from emonTx node with power1 with value 10.

[Follow install guide]( https://github.com/emoncms/emoncms/blob/master/docs/RaspberryPi/MQTT.md)

	sudo service mqtt_input start

[Forum Thread discussing emonhub MQTT support](http://openenergymonitor.org/emon/node/12091)

*Previously EmonHub posted decoded data to MQTT topic with CSV format to  `emonhub/rx/5/values 10,11,12` which the Nodes module suscribed to and decoded. Using the MQTT input service in conjunction with the new EmonHub MQTT node topic structure depreciates the Nodes module*

# Lightwave RF MQTT service

[Follow LightWave RF MQTT GitHub Repo Install Guide](https://github.com/openenergymonitor/lightwaverf-pi)

***

# Configure Firewall

Apache web server `sudo ufw allow 80/tcp` and`sudo ufw allow 443/tcp`

SSH server: `sudo ufw allow 22/tcp`

Mosquitto MQTT: `sudo ufw allow 1883/tcp`

Redis: `sudo ufw allow 6379/tcp`

OpenHAB: `sudo ufw allow 8080/tcp`

NodeRed: `sudo ufw allow 1880/tcp`

	sudo ufw enable

***

# Install NodeRED

[Follow OEM NodeRED install guide](https://github.com/openenergymonitor/oem_node-red) with OEM examples.

Default flows admin user: `emonpi` and password `emonpi2016`

***

# Install openHAB

[Follow OEM openHAB install guide](https://github.com/openenergymonitor/oem_openHab) with OEM examples

# GSM HiLink Huawei USB modem dongle support

[Follow Huawei Hi-Link RasPi setup guide](https://github.com/openenergymonitor/huawei-hilink-status/blob/master/README.md) to setup HiLink devices and useful status utility. The emonPiLCD now uses the same Huawei API to display GSM / 3G connection status and signal level on the LCD.

# emonPi Setup Wifi AP mode

Wifi Access Point mode is useful when using emonPi without a interent connection of with a 3G dongle.

[Follow guide to install hostpad and DHCP](https://github.com/openenergymonitor/emonpi/blob/master/docs/wifiAP.md)

Including installing start/ stop script to start the AP and also bridge the 3G dongle interface on eth1 to wlan0


***

# Symlink scripts

## emonSDexpand:

`sudo ln -s /home/pi/usefulscripts/sdpart/sdpart_imagefile /sbin/emonSDexpand`

## Fstab:

`sudo ln -s /home/pi/emonpi/fstab /etc/fstab`

***

# Clean Up

## Remove unused packages

`$ sudo apt-get clean all`

## Run Factory reset

```
sudo su
/home/pi/emonpi/.factoryreset
```
