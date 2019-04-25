#!/bin/bash
source config.ini

echo "-------------------------------------------------------------"
echo "Redis configuration"
echo "-------------------------------------------------------------"
sudo apt-get install -y redis-server

if [ "$install_php" = true ]; then 
    echo "-------------------------------------------------------------"
    printf "\n" | sudo pecl install redis
    echo "-------------------------------------------------------------"

    # Add redis to php mods available 
    printf "extension=redis.so" | sudo tee /etc/php/7.0/mods-available/redis.ini 1>&2
    sudo phpenmod redis
fi

sudo pip install redis

# Disable redis persistance
sudo sed -i "s/^save 900 1/#save 900 1/" /etc/redis/redis.conf
sudo sed -i "s/^save 300 1/#save 300 1/" /etc/redis/redis.conf
sudo sed -i "s/^save 60 1/#save 60 1/" /etc/redis/redis.conf
sudo systemctl restart redis-server
