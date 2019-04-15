# emonSD image build script

**Todo 1st release**

- emoncms module branch options
- ext2 data partition, mount /var/opt/emon as ext2?
- review log2ram

**Todo 2nd release**

- fix flexible emoncms_core install location (currently /var/www/emoncms symlinked to /var/www/html)
- review emoncms logfile location
- review /var/www/html/emoncms symlink

The following build script can be used to build a fully fledged emoncms installation on debian operating systems, including: installation of LAMP server and related packages, redis, mqtt, emoncms core, emoncms modules, emonhub and if applicable: raspberrypi support for serial port and wifi access point.

Tested on: 

- [Raspbian Stretch Lite](https://www.raspberrypi.org/downloads/raspbian/), Version: November 2018, Release date: 2018-11-13

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



