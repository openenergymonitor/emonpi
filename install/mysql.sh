#!/bin/bash
source config.ini

echo "-------------------------------------------------------------"
echo "Setup the Mariadb server (MYSQL)"
echo "-------------------------------------------------------------"
sudo apt-get install -y mariadb-server mysql-client

# Secure mysql
echo "- Secure MYSQL"
sudo mysql -e "DELETE FROM mysql.user WHERE User='root' AND Host NOT IN ('localhost', '127.0.0.1', '::1'); DELETE FROM mysql.user WHERE User=''; DROP DATABASE IF EXISTS test; DELETE FROM mysql.db WHERE Db='test' OR Db='test\_%'; FLUSH PRIVILEGES;"
# Create the emoncms database using utf8 character decoding:
echo "- Create $mysql_database database"
sudo mysql -e "CREATE DATABASE $mysql_database DEFAULT CHARACTER SET utf8;"
# Add emoncms database, set user permissions
echo "- Add user:$mysql_user and assign to database:$mysql_database"
sudo mysql -e "CREATE USER '$mysql_user'@'localhost' IDENTIFIED BY '$mysql_password'; GRANT ALL ON $mysql_database.* TO '$mysql_user'@'localhost'; flush privileges;"

# Allow MYSQL db to be located in home folder /home/pi/data
echo "Edit mariadb service file to allow db to be located in ~/data"
sudo mkdir -p /etc/systemd/system/mariadb.service.d
sudo cp /home/pi/emonpi/stretch/mariadb-service-d-override.conf /etc/systemd/system/mariadb.service.d/override.conf

echo "Reload mariadb service"
sudo systemctl daemon-reload
sudo systemctl restart mariadb
