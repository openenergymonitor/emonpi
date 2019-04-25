#!/bin/bash
source config.ini

echo "-------------------------------------------------------------"
echo "Apache configuration"
echo "-------------------------------------------------------------"
sudo apt-get install -y apache2

# Enable apache mod rewrite
sudo a2enmod rewrite
sudo cat <<EOF >> /tmp/emoncms.conf
<Directory /var/www/html/emoncms>
    Options FollowSymLinks
    AllowOverride All
    DirectoryIndex index.php
    Order allow,deny
    Allow from all
</Directory>
EOF
sudo mv /tmp/emoncms.conf /etc/apache2/sites-available/emoncms.conf
# Review is this line needed? if so check for existing entry
# printf "ServerName localhost" | sudo tee -a /etc/apache2/apache2.conf 1>&2
sudo a2ensite emoncms

# Disable apache2 access logs
sudo sed -i "s/^\tCustomLog/\t#CustomLog/" /etc/apache2/sites-available/000-default.conf
sudo sed -i "s/^CustomLog/#CustomLog/" /etc/apache2/conf-available/other-vhosts-access-log.conf

sudo systemctl restart apache2
