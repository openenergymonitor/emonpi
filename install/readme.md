# emonSD image build script

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


