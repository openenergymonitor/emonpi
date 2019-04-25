# emonSD image build script

**Todo 1st release**

- finalise emoncms log locations i.e: /var/log/emon/emoncms/sync.log
- update emoncms & modules with configurable log directory paths for sync/update etc.
- finalise logrotate config
- move install and update from emonpi repo to emonscripts or other suitable name

**Todo 2nd release**

- fix flexible emoncms_core install location (currently /var/www/emoncms symlinked to /var/www/html)
- review /var/www/html/emoncms symlink

The following build script can be used to build a fully fledged emoncms installation on debian operating systems, including: installation of LAMP server and related packages, redis, mqtt, emoncms core, emoncms modules, emonhub and if applicable: raspberrypi support for serial port and wifi access point.

Tested on: 

- [Raspbian Stretch Lite](https://www.raspberrypi.org/downloads/raspbian/), Version: November 2018, Release date: 2018-11-13

**Before starting it is recommended to create an ext2 partition for emoncms data, see below.**

Run default install:

    wget https://raw.githubusercontent.com/openenergymonitor/emonpi/master/install/init.sh
    chmod +x init.sh
    ./init.sh
    
### Configure install

The default configuration is for the RaspberryPi platform and Raspbian Stretch image specifically. To run the installation on a different distribution, you may need to change the configuration to reflect the target environment.

See explanation and settings in installation configuration file here: [config.ini](https://github.com/openenergymonitor/emonpi/blob/master/install/config.ini)

### Running scripts individually

The installation process is broken out into seperate scripts that can be run individually.

---

**[init.sh:](https://github.com/openenergymonitor/emonpi/blob/master/install/init.sh)** Launches the full installation script, first downloading the 'emonpi' repository that contains the rest of the installation scripts.

**[main.sh:](https://github.com/openenergymonitor/emonpi/blob/master/install/main.sh)** Loads the configuration file and runs the individual installation scripts as applicable.

---

**[redis.sh:](https://github.com/openenergymonitor/emonpi/blob/master/install/redis.sh)** Installs redis and configures the redis configuration file: turning off redis database persistance.

**[mosquitto.sh:](https://github.com/openenergymonitor/emonpi/blob/master/install/mosquitto.sh)** Installation and configuration of mosquitto MQTT server, used for emoncms MQTT interface with emonhub and smart control e.g: demandshaper module.

**[apache.sh:](https://github.com/openenergymonitor/emonpi/blob/master/install/apache.sh)** Apache configuration, mod rewrite and apache logging.

**[mysql.sh:](https://github.com/openenergymonitor/emonpi/blob/master/install/mysql.sh)** Removal of test databases, creation of emoncms database and emoncms mysql user.

**[emoncms_core.sh:](https://github.com/openenergymonitor/emonpi/blob/master/install/emoncms_core.sh)** Installation of emoncms core, data directories and emoncms core services.

**[emoncms_modules.sh:](https://github.com/openenergymonitor/emonpi/blob/master/install/emoncms_modules.sh)** Installation of emoncms optional modules listed in config.ini e.g: Graphs, Dashboards, Apps & Backup

**[emonhub.sh:](https://github.com/openenergymonitor/emonpi/blob/master/install/emonhub.sh)** Emonhub is used in the OpenEnergyMonitor system to read data received over serial from either the EmonPi board or the RFM12/69Pi adapter board then forward the data to emonCMS in a decoded ready-to-use form

**[firmware.sh:](https://github.com/openenergymonitor/emonpi/blob/master/install/firmware.sh)** Requirements for firmware upload to directly connected emonPi hardware or rfm69pi adapter board.

**[emonpilcd.sh:](https://github.com/openenergymonitor/emonpi/blob/master/install/emonpilcd.sh)** Support for emonPi LCD.

**[wifiap.sh:](https://github.com/openenergymonitor/emonpi/blob/master/install/wifiap.sh)** RaspberryPi 3B+ WIFI Access Point support.

**[emonsd.sh:](https://github.com/openenergymonitor/emonpi/blob/master/install/emonsd.sh)** RaspberryPi specific configuration e.g: logging, default SSH password and hostname.

### Setup ext2 data partition

Use a partition editor to resize the raspbian stretch OS partition, select 3-4GB for the OS partition and expand the new partition to the remaining space.

Steps for creating 3rd partition for data using fdisk and mkfs:

    sudo fdisk -l
    Note end of last partition (5785599 on standard sd card)
    sudo fdisk /dev/mmcblk0
    enter: n->p->3
    enter: 5785600
    enter: default or 7626751
    enter: w (write partition to disk)
    fails with error, will write at reboot
    sudo reboot

On reboot, login and run:

    sudo mkfs.ext2 -b 1024 /dev/mmcblk0p3

*Note: We create here an ext2 filesystem with a blocksize of 1024 bytes instead of the default 4096 bytes. A lower block size results in significant write load reduction when using an application like emoncms that only makes small but frequent and across many files updates to disk. Ext2 is choosen because it supports multiple linux user ownership options which are needed for the mysql data folder. Ext2 is non-journaling which reduces the write load a little although it may make data recovery harder vs Ext4, The data disk size is small however and the downtime from running fsck is perhaps less critical.*

Create a directory that will be a mount point for the rw data partition

    sudo mkdir /var/opt/emon
    sudo chwon www-data /var/opt/emon

Use modified fstab

    wget https://raw.githubusercontent.com/openenergymonitor/emonpi/master/install/fstab
    sudo cp fstab /etc/fstab
    sudo reboot

