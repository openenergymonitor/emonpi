#!/bin/bash

# Emoncms V9 emonPi Install 

# -----------------------------------------------------------------------------------------------
# INSTALL APT-GET DEPENDENCIES
# -----------------------------------------------------------------------------------------------
# rpi-rw

# Install all dependencies
# There are a few extra things in here such as mosquitto
# which will become useful soon

# MYSQL INSTALL WITHOUT PASSWORD PROMPT
echo "MYSQL INSTALL WITHOUT PASSWORD PROMPT"
sudo debconf-set-selections <<< 'mysql-server mysql-server/root_password password raspberry'
sudo debconf-set-selections <<< 'mysql-server mysql-server/root_password_again password raspberry'
sudo apt-get -y install mysql-server

# All other apt-get packages
echo "Install required packages"
sudo apt-get install -y apache2 mysql-client php5 libapache2-mod-php5 php5-mysql php5-curl php-pear php5-dev php5-mcrypt git-core redis-server build-essential ufw ntp python-serial python-configobj python-pip python-dev screen sysstat minicom
# Don't install mosquitto from Debian PPA, install newer version from mosquitto PPA. See emonHub install https://github.com/openenergymonitor/emonhub 

sed 's/\/var\/log\/apache2/\/var\/log/' /etc/apache2/envvars > /tmp/apache2envvars
sudo cp /tmp/apache2envvars /etc/apache2/envvars
sudo service apache2 restart

# Redis and log4php
echo "Pear install Redis and log4php"
sudo pear channel-discover pear.swiftmailer.org
sudo pecl install channel://pecl.php.net/dio-0.0.6 redis swift/swift log4php
sudo pear install log4php/Apache_log4php

# Add pecl modules to php5 config
echo "Add pecl modules to php5 config"
sudo sh -c 'echo "extension=dio.so" > /etc/php5/apache2/conf.d/20-dio.ini'
sudo sh -c 'echo "extension=dio.so" > /etc/php5/cli/conf.d/20-dio.ini'
sudo sh -c 'echo "extension=redis.so" > /etc/php5/apache2/conf.d/20-redis.ini'
sudo sh -c 'echo "extension=redis.so" > /etc/php5/cli/conf.d/20-redis.ini'

# MOD REWRITE
echo "MOD REWRITE"
sudo a2enmod rewrite

# Change allow override
echo "Change 'AllowOverride All' apache2 setting"
sed '/<Directory \/var\/www\/>/,/<\/Directory>/ s/AllowOverride None/AllowOverride All/' /etc/apache2/apache2.conf > /tmp/apache2.conf
sudo cp /tmp/apache2.conf /etc/apache2/apache2.conf

# Custom log
echo "custom log"
sed 's/CustomLog/# CustomLog/' /etc/apache2/apache2.conf > /tmp/apache2.conf
sudo cp /tmp/apache2.conf /etc/apache2/sites-enabled/apache2.conf

# Custom log vhosts
echo "custom log vhosts"
sed 's/CustomLog/# CustomLog/' /etc/apache2/conf.d/other-vhosts-access-log > /tmp/other-vhosts-access-log
sudo cp /tmp/other-vhosts-access-log /etc/apache2/conf.d/other-vhosts-access-log

# Comment out save settings in redis
echo "Comment out save settings in redis"
sed 's/logfile \/var\/log\/redis\/redis-server.log/# logfile \/var\/log\/redis\/redis-server.log/' /etc/redis/redis.conf > /tmp/redis.conf
sudo cp /tmp/redis.conf /etc/redis/redis.conf

sed 's/save 900 1/# save 900 1/' /etc/redis/redis.conf > /tmp/redis.conf
sudo cp /tmp/redis.conf /etc/redis/redis.conf
sed 's/save 300 10/# save 300 10/' /etc/redis/redis.conf > /tmp/redis.conf
sudo cp /tmp/redis.conf /etc/redis/redis.conf
sed 's/save 60 10000/# save 60 10000/' /etc/redis/redis.conf > /tmp/redis.conf
sudo cp /tmp/redis.conf /etc/redis/redis.conf

# MYSQL database
echo "create emoncms MYSQL database"
mysql -u root -e "create database emoncms" -praspberry

# Create data repositories for emoncms feed engine's
echo "Create data repositories for emoncms feed engine's"
sudo mkdir /home/pi/data
sudo mkdir /home/pi/data/phpfina
sudo mkdir /home/pi/data/phptimeseries
sudo chown www-data:root /home/pi/data/phpfina
sudo chown www-data:root /home/pi/data/phptimeseries

echo "git clone emoncms repo into /var/www/"
sudo chown pi /var/www
cd /var/www
git clone -b stable https://github.com/emoncms/emoncms.git

echo "Use default.emonpi.settings.php as settings.php"
cp /var/www/emoncms/default.emonpi.settings.php /var/www/emoncms/settings.php

echo "git clone emoncms modules repo into /var/www/Modules"
cd /var/www/emoncms/Modules
git clone https://github.com/emoncms/nodes.git
cd /var/www/emoncms/Modules/app && git checkout 9.0

cd /var/www/emoncms/Modules
git clone https://github.com/emoncms/app.git
cd /var/www/emoncms/Modules/nodes && git checkout 9.0

cd /var/www/emoncms/Modules
git clone https://github.com/emoncms/config.git
cd /var/www/emoncms/Modules/config && git checkout 9.0

cd /var/www/emoncms/Modules
git clone https://github.com/emoncms/wifi.git
cd /var/www/emoncms/Modules/wifi && git checkout 9.0

cd
rm /var/www/emoncms/Modules/input/input_menu.php

sudo mv /etc/wpa_supplicant/wpa_supplicant.conf /home/pi/data
sudo ln -s /home/pi/data/wpa_supplicant.conf /etc/wpa_supplicant/wpa_supplicant.conf

echo "Add wifi commands from 'wifi' file to sudoers.d"
if [ ! -f /etc/sudoers.d/wifi ]; then 
	sudo mv /home/pi/emonpi/wifi /etc/sudoers.d/wifi
	sudo chown root:root /etc/sudoers.d/wifi
	sudo chmod 0440
fi

echo "Add wifi-check script to /user/local/bin"
sudo cp /home/pi/emonpi/wifi-check /usr/local/bin/wifi-check
sudo chmod +x /usr/local/bin/wifi-check

echo "Install wifi-check script crontab entry, every 5 min"
# Add crontab entry to run wifi check script ever 5min
crontab -l > mycron #write out current crontab
# Check crontab entry does not already exist, if not add new entry
if ! grep -Fxq "*/5 * * * * /usr/local/bin/wifi-check > /tmp/wificheck.log 2>&1" mycron ; then
        echo " */5 * * * * /usr/local/bin/wifi-check > /tmp/wificheck.log 2>&1" >> mycron  #echo new cron into cronfile
        sudo crontab mycron #install new cron file
fi
rm mycron

echo "Install Emoncms update script cron entry, every 1 min. Script exists unless flag exists in /tmp"
crontab -l > mycron #write out current crontab
# Check crontab entry does not already exist, if not add new entry
if ! grep -Fxq " * * * * * /home/pi/emonpi/update >> /home/pi/data/emonpiupdate.log 2>&1" mycron ; then
        echo " * * * * * /home/pi/emonpi/update >> /home/pi/data/emonpiupdate.log 2>&1" >> mycron  #echo new cron into cronfile
        crontab mycron #install new cron file
fi
rm mycron


echo "Add rc.local entry to run factory update on first boot"
if ! grep -Fxq "/home/pi/emonpi/./firstbootupdate" /etc/rc.local; then
	sed -i -e '$i \/home/pi/emonpi/./firstbootupdate\n' /etc/rc.local



echo "move emoncms.conf to RW partition home/pi/data"
cd /home/pi
> emoncms.conf
sudo mv /home/pi/emoncms.conf /home/pi/data/emoncms.conf
sudo chown pi:www-data /home/pi/data/emoncms.conf
sudo chmod 664 /home/pi/data/emoncms.conf

echo "install emoncms feedwriter script to run at boot in /etc/init.d"
cd /etc/init.d && sudo ln -s /var/www/emoncms/scripts/feedwriter
sudo chown root:root /var/www/emoncms/scripts/feedwriter
sudo chmod 755 /var/www/emoncms/scripts/feedwriter
sudo update-rc.d feedwriter defaults

sudo rm /var/www/index.html
echo "set emoncms folder http redirect"
echo "<?php header('Location: ../emoncms'); ?>" > /var/www/index.php

echo "create emonpi update logfile"
sudo touch /home/pi/data/emonpiupdate.log
sudo chown pi:www-data /home/pi/data/emonpiupdate.log
sudo chmod 666 /home/pi/data/emonpiupdate.log

echo "create emoncms logfile"
sudo touch /var/log/emoncms.log
sudo chmod 666 /var/log/emoncms.log
  

#echo "Installing Emonhub"
#cd
#git clone https://github.com/openenergymonitor/emonhub.git && emonhub/install
sudo service emonhub stop
sudo chown pi:www-data /home/pi/data/emonhub.conf
sudo chmod 664 /home/pi/data/emonhub.conf
sudo service emonhub start

sudo cp /var/www/emoncms/Modules/nodes/emoncms-nodes-service /etc/init.d/
sudo chmod 755 /etc/init.d/emoncms-nodes-service
sudo update-rc.d emoncms-nodes-service defaults
sudo service emoncms-nodes-service start

# Already done by emonPi install script 
#sudo wget https://raw.github.com/lurch/rpi-serial-console/master/rpi-serial-console -O /usr/bin/rpi-serial-console && sudo chmod +x /usr/bin/rpi-serial-console
#sudo rpi-serial-console disable
#rpi-serial-console status

echo "restart redis"
sudo service redis-server restart
echo "restart apache2"
sudo service apache2 restart
echo "Emoncms install script done!"
