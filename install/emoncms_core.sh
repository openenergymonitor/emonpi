#! /bin/sh
source config.ini

echo "-------------------------------------------------------------"
echo "Install Emoncms Core"
echo "-------------------------------------------------------------"
# Emoncms install
# Give pi user ownership over /var/www/ folder
sudo chown $user /var/www
cd /var/www && git clone -b master https://github.com/emoncms/emoncms.git

# Create logfile
sudo touch /var/log/emoncms.log
sudo chmod 666 /var/log/emoncms.log

# Configure emoncms database settings
# Make a copy of default.settings.php and call it settings.php:
cp $usrdir/emonpi/install/default.emonpi.settings.php /var/www/emoncms/settings.php

# Create data repositories for emoncms feed engines:
echo " - Creating phpfina, phpiwa, phptimeseries emoncms feed directories"
sudo mkdir /var/lib/phpfiwa
sudo mkdir /var/lib/phpfina
sudo mkdir /var/lib/phptimeseries
sudo chown www-data:root /var/lib/phpfiwa
sudo chown www-data:root /var/lib/phpfina
sudo chown www-data:root /var/lib/phptimeseries

# Create a symlink to reference emoncms within the web root folder:
cd /var/www/html && sudo ln -s /var/www/emoncms

# Redirect
echo "<?php header('Location: ../emoncms'); ?>" > /home/pi/index.php
sudo mv /home/pi/index.php /var/www/html/index.php
sudo rm /var/www/html/index.html

echo "-------------------------------------------------------------"
echo "Install Emoncms Services"
echo "-------------------------------------------------------------"
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

# Sudoers entry (review)
sudo visudo -cf $usrdir/emonpi/emoncms-rebootbutton && \
sudo cp $usrdir/emonpi/emoncms-rebootbutton /etc/sudoers.d/
sudo chmod 0440 /etc/sudoers.d/emoncms-rebootbutton
echo "Install emonPi Emoncms admin reboot button sudoers entry"

sudo visudo -cf $usrdir/emonpi/emoncms-setup/emoncms-setup-sudoers && \
sudo cp $usrdir/emonpi/emoncms-setup/emoncms-setup-sudoers /etc/sudoers.d/
sudo chmod 0440 /etc/sudoers.d/emoncms-setup-sudoers
echo "Emoncms setup module sudoers entry installed"
