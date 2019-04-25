#!/bin/bash
source config.ini

echo "-------------------------------------------------------------"
echo "Install PHP"
echo "-------------------------------------------------------------"

sudo apt-get install -y php7.0

if [ "$install_apache" = true ]; then
    sudo apt-get install -y libapache2-mod-php7.0
fi

if [ "$install_mysql" = true ]; then
    sudo apt-get install -y php7.0-mysql
fi

sudo apt-get install -y php7.0-gd php7.0-opcache php7.0-curl php-pear php7.0-dev php7.0-mcrypt php7.0-common php7.0-mbstring

sudo pecl channel-update pecl.php.net
