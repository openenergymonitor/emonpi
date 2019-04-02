# --------------------------------------------------------------------------------
# RaspberryPi Strech Build Script
# Emoncms, Emoncms Modules, EmonHub & dependencies
#
# Tested with: Raspbian Strech
# Date: 19 March 2019
#
# Status: Work in Progress
# --------------------------------------------------------------------------------

# Review splitting this up into seperate scripts
# - emoncms installer
# - emonhub installer

# Format as documentation

#! /bin/sh

# auto detect install location (e.g /usr/emoncms or /home/pi)
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
usrdir=${DIR/\/emonpi\/install/}

USER=pi
DEFAULT_SSH_PASSWORD=emonpi2016
hostname=emonpi
emonSD_pi_env=1

mysql_user=emonpi
mysql_password=emonpiemoncmsmysql2016

mqtt_user=emonpi
mqtt_password=emonpimqtt2016

sudo apt-get update -y
# sudo apt-get upgrade -y
sudo apt-get -y dist-upgrade
sudo apt-get clean

# Needed on stock raspbian lite 19th March 2019
sudo apt --fix-broken install

# Emoncms install process from:
# https://github.com/emoncms/emoncms/blob/master/docs/RaspberryPi/readme.md
sudo apt-get install -y apache2 mariadb-server mysql-client php7.0 libapache2-mod-php7.0 php7.0-mysql php7.0-gd php7.0-opcache php7.0-curl php-pear php7.0-dev php7.0-mcrypt php7.0-common git build-essential php7.0-mbstring python-pip python-dev gettext

# Install the pecl dependencies
sudo pecl channel-update pecl.php.net

$usrdir/emonpi/install/redis.sh
$usrdir/emonpi/install/mosquitto.sh $mqtt_user $mqtt_password
$usrdir/emonpi/install/apache.sh $usrdir
$usrdir/emonpi/install/mysql.sh $mysql_user $mysql_password
$usrdir/emonpi/install/emoncms_core.sh
$usrdir/emonpi/install/emoncms_modules.sh
$usrdir/emonpi/install/emonhub.sh $emonSD_pi_env

if [ "$emonSD_pi_env" = "1" ]; then
    $usrdir/emonpi/install/emonpilcd.sh
    $usrdir/emonpi/install/emonsd.sh
    
    # Enable service-runner update
    # emonpi update checks for image type and only runs with a valid image name file in the boot partition
    sudo touch /boot/emonSD-30Oct18
    
    # Reboot to complete
    sudo reboot
fi
