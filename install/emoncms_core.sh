#!/bin/bash
source config.ini

echo "-------------------------------------------------------------"
echo "Install Emoncms Core"
echo "-------------------------------------------------------------"

# Emoncms install
# Give pi user ownership over /var/www/ folder
sudo chown $user /var/www
if [ ! -d $emoncms_www ]; then
    cd /var/www && git clone -b master https://github.com/emoncms/emoncms.git
else
    echo "- emoncms already installed"
fi

# Create logfile
echo "- creating emoncms log file"
sudo touch /var/log/emoncms.log
sudo chmod 666 /var/log/emoncms.log

# Configure emoncms database settings
# Make a copy of default.settings.php and call it settings.php:
echo "- installing default emoncms settings.php"
cp $usrdir/emonpi/install/default.emonpi.settings.php /var/www/emoncms/settings.php

# Create data repositories for emoncms feed engines:
for engine in "phpfina" "phpfiwa" "phptimeseries"; do
    if [ ! -d /var/lib/$engine ]; then
        echo "- create $engine dir"
        sudo mkdir /var/lib/$engine
        sudo chown www-data:root /var/lib/$engine
    else
        echo "- datadir $engine already exists"
    fi
done


# Create a symlink to reference emoncms within the web root folder:
if [ ! -d /var/www/html/emoncms ]; then
    cd /var/www/html && sudo ln -s /var/www/emoncms
fi

# Redirect
echo "<?php header('Location: ../emoncms'); ?>" > /home/pi/index.php
sudo mv /home/pi/index.php /var/www/html/index.php
if [ -f /var/www/html/emoncms ]; then
    sudo rm /var/www/html/index.html
fi

echo "-------------------------------------------------------------"
echo "Install Emoncms Services"
echo "-------------------------------------------------------------"
for service in "emoncms_mqtt" "feedwriter" "service-runner"; do
    servicepath=/var/www/emoncms/scripts/services/$service/$service.service
    if [ ! -f /lib/systemd/system/$service.service ]; then
        echo "- Installing $service.service in /lib/systemd/system (creating symlink)"
        sudo ln -s $servicepath /lib/systemd/system
        sudo systemctl enable $service.service
        sudo systemctl start $service.service
    else
        echo "- $service.service already installed"
    fi
done

# Sudoers entry (review)
sudo visudo -cf $usrdir/emonpi/sudoers.d/emoncms-rebootbutton && \
sudo cp $usrdir/emonpi/sudoers.d/emoncms-rebootbutton /etc/sudoers.d/
sudo chmod 0440 /etc/sudoers.d/emoncms-rebootbutton
echo "- Install emonPi Emoncms admin reboot button sudoers entry"

sudo visudo -cf $usrdir/emonpi/sudoers.d/emoncms-setup-sudoers && \
sudo cp $usrdir/emonpi/sudoers.d/emoncms-setup-sudoers /etc/sudoers.d/
sudo chmod 0440 /etc/sudoers.d/emoncms-setup-sudoers
echo "- Emoncms setup module sudoers entry installed"
