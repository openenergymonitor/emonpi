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

USER=pi
homedir=/home/pi
DEFAULT_SSH_PASSWORD=emonpi2016
hostname=emonpi

sudo apt-get update -y
# sudo apt-get upgrade -y
sudo apt-get -y dist-upgrade
sudo apt-get clean

# Needed on stock raspbian lite 19th March 2019
sudo apt --fix-broken install

# Emoncms install process from:
# https://github.com/emoncms/emoncms/blob/master/docs/RaspberryPi/readme.md
sudo apt-get install -y apache2 mariadb-server mysql-client php7.0 libapache2-mod-php7.0 php7.0-mysql php7.0-gd php7.0-opcache php7.0-curl php-pear php7.0-dev php7.0-mcrypt php7.0-common redis-server git build-essential php7.0-mbstring libmosquitto-dev mosquitto python-pip python-dev gettext ufw

# Install the pecl dependencies
sudo pecl channel-update pecl.php.net
printf "\n" | sudo pecl install redis Mosquitto-beta

# --------------------------------------------------------------------------------
# Redis configuration
# --------------------------------------------------------------------------------

# Add redis to php mods available 
printf "extension=redis.so" | sudo tee /etc/php/7.0/mods-available/redis.ini 1>&2
sudo phpenmod redis

# Disable redis persistance
sudo sed -i "s/^save 900 1/#save 900 1/" /etc/redis/redis.conf
sudo sed -i "s/^save 300 1/#save 300 1/" /etc/redis/redis.conf
sudo sed -i "s/^save 60 1/#save 60 1/" /etc/redis/redis.conf
sudo service redis-server restart

# --------------------------------------------------------------------------------
# Mosquitto configuration
# --------------------------------------------------------------------------------

# Add mosquitto to php mods available
printf "extension=mosquitto.so" | sudo tee /etc/php/7.0/mods-available/mosquitto.ini 1>&2
sudo phpenmod mosquitto

# Disable mosquitto persistance
sudo sed -i "s/^persistence true/persistence false/" /etc/mosquitto/mosquitto.conf
# append line: allow_anonymous false
sudo sed -i -n '/allow_anonymous false/!p;$a allow_anonymous false' /etc/mosquitto/mosquitto.conf
# append line: password_file /etc/mosquitto/passwd
sudo sed -i -n '/password_file \/etc\/mosquitto\/passwd/!p;$a password_file \/etc\/mosquitto\/passwd' /etc/mosquitto/mosquitto.conf
# append line: log_type error
sudo sed -i -n '/log_type error/!p;$a log_type error' /etc/mosquitto/mosquitto.conf

# Create mosquitto password file
sudo touch /etc/mosquitto/passwd
sudo mosquitto_passwd -b /etc/mosquitto/passwd emonpi emonpimqtt2016

# --------------------------------------------------------------------------------
# Apache configuration
# --------------------------------------------------------------------------------

# Enable apache mod rewrite
sudo a2enmod rewrite
cat <<EOF >> $homedir/emoncms.conf
<Directory /var/www/html/emoncms>
    Options FollowSymLinks
    AllowOverride All
    DirectoryIndex index.php
    Order allow,deny
    Allow from all
</Directory>
EOF
sudo mv $homedir/emoncms.conf /etc/apache2/sites-available/emoncms.conf
# Review is this line needed? if so check for existing entry
# printf "ServerName localhost" | sudo tee -a /etc/apache2/apache2.conf 1>&2
sudo a2ensite emoncms

# Disable apache2 access logs
sudo sed -i "s/^\tCustomLog/\t#CustomLog/" /etc/apache2/sites-available/000-default.conf
sudo sed -i "s/^CustomLog/#CustomLog/" /etc/apache2/conf-available/other-vhosts-access-log.conf

sudo service apache2 restart

# --------------------------------------------------------------------------------
# Setup the Mariadb server (MYSQL)
# --------------------------------------------------------------------------------
# Secure mysql
sudo mysql -e "DELETE FROM mysql.user WHERE User='root' AND Host NOT IN ('localhost', '127.0.0.1', '::1'); DELETE FROM mysql.user WHERE User=''; DROP DATABASE IF EXISTS test; DELETE FROM mysql.db WHERE Db='test' OR Db='test\_%'; FLUSH PRIVILEGES;"
# Create the emoncms database using utf8 character decoding:
sudo mysql -e "CREATE DATABASE emoncms DEFAULT CHARACTER SET utf8;"
# Add emoncms database, set user permissions
sudo mysql -e "CREATE USER 'emoncms'@'localhost' IDENTIFIED BY 'emonpiemoncmsmysql2016'; GRANT ALL ON emoncms.* TO 'emoncms'@'localhost'; flush privileges;"

# Create data repositories for emoncms feed engines:
sudo mkdir /var/lib/phpfiwa
sudo mkdir /var/lib/phpfina
sudo mkdir /var/lib/phptimeseries
sudo chown www-data:root /var/lib/phpfiwa
sudo chown www-data:root /var/lib/phpfina
sudo chown www-data:root /var/lib/phptimeseries

# --------------------------------------------------------------------------------
# Install Emoncms Core
# --------------------------------------------------------------------------------

# Emoncms install
# Give pi user ownership over /var/www/ folder
sudo chown $USER /var/www
cd /var/www && git clone -b stable https://github.com/emoncms/emoncms.git

# Create logfile
sudo touch /var/log/emoncms.log
sudo chmod 666 /var/log/emoncms.log

# Configure emoncms database settings
# Make a copy of default.settings.php and call it settings.php:
cd /var/www/emoncms && cp default.emonpi.settings.php settings.php

# Temporary: Replace this with default settings file
sed -i "s/\/home\/pi\/data\/phpfina\//\/var\/lib\/phpfina\//" settings.php
sed -i "s/\/home\/pi\/data\/phptimeseries\//\/var\/lib\/phptimeseries\//" settings.php
sed -i "s/\/home\/pi\/data\/phpfiwa\//\/var\/lib\/phpfiwa\//" settings.php

# use /usr/emoncms/modules as symlinked modules directory
sed -i 's/$homedir = "\/home\/pi";/$homedir = "\/usr\/emoncms\/modules";/' settings.php

# Create a symlink to reference emoncms within the web root folder:
cd /var/www/html && sudo ln -s /var/www/emoncms

# Redirect
echo "<?php header('Location: ../emoncms'); ?>" > /home/pi/index.php
sudo mv /home/pi/index.php /var/www/html/index.php
sudo rm /var/www/html/index.html

# --------------------------------------------------------------------------------
# Install Emoncms Services
# --------------------------------------------------------------------------------

# Emoncms MQTT
sudo ln -s /var/www/emoncms/scripts/services/emoncms_mqtt/emoncms_mqtt.service /lib/systemd/system
sudo systemctl enable emoncms_mqtt.service
sudo systemctl start emoncms_mqtt.service
# Feedwriter
sudo ln -s /var/www/emoncms/scripts/services/feedwriter/feedwriter.service /lib/systemd/system
sudo systemctl enable feedwriter.service
sudo systemctl start feedwriter.service
# Service runner
sudo pip install redis
sudo ln -s /var/www/emoncms/scripts/services/service-runner/service-runner.service /lib/systemd/system
sudo systemctl enable service-runner.service
sudo systemctl start service-runner.service

# --------------------------------------------------------------------------------
# Install Emoncms Modules
# --------------------------------------------------------------------------------

# Review default branch: e.g stable
cd /var/www/emoncms/Modules
git clone https://github.com/emoncms/config.git
git clone https://github.com/emoncms/graph.git
git clone https://github.com/emoncms/dashboard.git
git clone https://github.com/emoncms/device.git
git clone https://github.com/emoncms/app.git
git clone https://github.com/emoncms/wifi.git

# Install emoncms modules that do not reside in /var/www/emoncms/Modules

sudo mkdir /usr/emoncms
sudo chown $USER /usr/emoncms
mkdir /usr/emoncms/modules

# emoncms-sync.log is written to data folder
# change to /var/log or use emoncms logger
mkdir /usr/emoncms/modules/data

cd /usr/emoncms/modules

# Rename emoncms module component to backup-module
git clone https://github.com/emoncms/backup.git
cd backup
git checkout multienv
cp default.emonpi.config.cfg config.cfg
sed -i "s/\/home\/pi\/backup/\/usr\/emoncms\/modules\/backup/" config.cfg
ln -s /usr/emoncms/modules/backup/backup /var/www/emoncms/Modules/backup

cd /usr/emoncms/modules
git clone https://github.com/emoncms/postprocess.git
ln -s /usr/emoncms/modules/postprocess/postprocess-module /var/www/emoncms/Modules/postprocess

git clone https://github.com/emoncms/demandshaper.git
ln -s /usr/emoncms/modules/demandshaper/demandshaper-module /var/www/emoncms/Modules/demandshaper

git clone https://github.com/emoncms/sync.git
ln -s /usr/emoncms/modules/sync/sync-module /var/www/emoncms/Modules/sync

cd /home/pi
git clone https://github.com/emoncms/usefulscripts.git

# Symlink emoncms module folders here...
# Review consistent approach here

sudo visudo -cf $homedir/emonpi/emoncms-rebootbutton && \
sudo cp $homedir/emonpi/emoncms-rebootbutton /etc/sudoers.d/
sudo chmod 0440 /etc/sudoers.d/emoncms-rebootbutton
echo "Install emonPi Emoncms admin reboot button sudoers entry"
        
sudo visudo -cf $homedir/emonpi/emonhub-sudoers && \
sudo cp $homedir/emonpi/emonhub-sudoers /etc/sudoers.d/
sudo chmod 0440 /etc/sudoers.d/emonhub-sudoers
echo "emonhub service control sudoers entry installed"

sudo visudo -cf $homedir/emonpi/wifi-sudoers && \
sudo cp $homedir/emonpi/wifi-sudoers /etc/sudoers.d/
sudo chmod 0440 /etc/sudoers.d/wifi-sudoers
echo "wifi sudoers entry installed"

sudo visudo -cf $homedir/emonpi/emoncms-setup/emoncms-setup-sudoers && \
sudo cp $homedir/emonpi/emoncms-setup/emoncms-setup-sudoers /etc/sudoers.d/
sudo chmod 0440 /etc/sudoers.d/emoncms-setup-sudoers
echo "Emoncms setup module sudoers entry installed"

# --------------------------------------------------------------------------------
# Install EmonHub
# --------------------------------------------------------------------------------
# RaspberryPi Serial configuration
# disable Pi3 Bluetooth and restore UART0/ttyAMA0 over GPIOs 14 & 15;
# Review should this be: dtoverlay=pi3-miniuart-bt?
sudo sed -i -n '/dtoverlay=pi3-disable-bt/!p;$a dtoverlay=pi3-disable-bt' /boot/config.txt

# We also need to stop the Bluetooth modem trying to use UART
sudo systemctl disable hciuart

# Remove console from /boot/cmdline.txt
sudo sed -i "s/console=serial0,115200 //" /boot/cmdline.txt

# stop and disable serial service??
sudo systemctl stop serial-getty@ttyAMA0.service
sudo systemctl disable serial-getty@ttyAMA0.service

cd /home/pi
git clone https://github.com/openenergymonitor/emonhub.git
mkdir data
sudo apt-get install -y python-serial python-configobj
sudo pip install paho-mqtt requests
cd /home/pi/emonhub
sudo ./install.systemd
sudo systemctl start emonhub.service

# Temporary: replace with update to default settings file
sed -i "s/loglevel = DEBUG/loglevel = WARNING/" /home/pi/data/emonhub.conf

# --------------------------------------------------------------------------------
# EmonPi repo
# --------------------------------------------------------------------------------
cd /home/pi/
git clone https://github.com/openenergymonitor/emonpi.git
git clone https://github.com/openenergymonitor/RFM2Pi


# --------------------------------------------------------------------------------
# EmonPi LCD Support
# --------------------------------------------------------------------------------
sudo apt-get install -y python-smbus i2c-tools python-rpi.gpio python-gpiozero
sudo pip install xmltodict

# Uncomment dtparam=i2c_arm=on
sudo sed -i "s/^#dtparam=i2c_arm=on/dtparam=i2c_arm=on/" /boot/config.txt
# Append line i2c-dev to /etc/modules
sudo sed -i -n '/i2c-dev/!p;$a i2c-dev' /etc/modules

# Enable service-runner update
# emonpi update checks for image type and only runs with a valid image name file in the boot partition
sudo touch /boot/emonSD-30Oct18

# Try running emoncms Update
# !!!! avrdude and emonPiLCD still to install !!!! 

# --------------------------------------------------------------------------------
# Install log2ram, so that logging is on RAM to reduce SD card wear.
# Logs are written to disk every hour or at shutdown
# --------------------------------------------------------------------------------
# Review: @pb66 modifications, rotated logs are moved out of ram&sync cycle
curl -Lo log2ram.tar.gz https://github.com/azlux/log2ram/archive/master.tar.gz
tar xf log2ram.tar.gz
cd log2ram-master
chmod +x install.sh && sudo ./install.sh
cd ..
rm -r log2ram-master

# --------------------------------------------------------------------------------
# Enable serial uploads with avrdude and autoreset
# --------------------------------------------------------------------------------
cd
git clone https://github.com/openenergymonitor/avrdude-rpi.git ~/avrdude-rpi && ~/avrdude-rpi/install

# --------------------------------------------------------------------------------
# Misc
# --------------------------------------------------------------------------------
# Review: provide configuration file for default password and hostname

# Set default SSH password:
printf "raspberry\n$DEFAULT_SSH_PASSWORD\n$DEFAULT_SSH_PASSWORD" | passwd

# Set hostname
sudo sed -i "s/raspberrypi/$hostname/g" /etc/hosts
printf $hostname | sudo tee /etc/hostname > /dev/null

# Configure UFW firewall
# Review: reboot required before running:
# sudo ufw allow 80/tcp
# sudo ufw allow 443/tcp (optional, HTTPS not present)
# sudo ufw allow 22/tcp
# sudo ufw allow 1883/tcp #(optional, Mosquitto)
# sudo ufw enable

# Disable duplicate daemon.log logging to syslog
sudo sed -i "s/*.*;auth,authpriv.none\t\t-\/var\/log\/syslog/*.*;auth,authpriv.none,daemon.none\t\t-\/var\/log\/syslog/" /etc/rsyslog.conf
sudo service rsyslog restart
# REVIEW: https://openenergymonitor.org/forum-archive/node/12566.html

# Review: Memory Tweak
# Append gpu_mem=16 to /boot/config.txt this caps the RAM available to the GPU. 
# Since we are running headless this will give us more RAM at the expense of the GPU
# gpu_mem=16

# Review: change elevator=deadline to elevator=noop
# sudo nano /boot/cmdline.txt
# see: https://github.com/openenergymonitor/emonpi/blob/master/docs/SD-card-build.md#raspi-serial-port-setup

# Review: Force NTP update 
# is this needed now that image is not read only?
# 0 * * * * /home/pi/emonpi/ntp_update.sh >> /var/log/ntp_update.log 2>&1

# Review automated install: Emoncms Language Support
# sudo dpkg-reconfigure locales

sudo reboot
