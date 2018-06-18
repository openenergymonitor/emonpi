## emonPi / emonBase Network setup wizard 

Network setup wizard on first-boot via Emoncms interface 

See blog post:

https://blog.openenergymonitor.org/2017/06/emonpi-network-setup-wizard/

### Install 

Setup sudoers entry

    sudo visudo -cf /home/pi/emonpi/emoncms-setup/emoncms-setup-sudoers && \
    sudo cp /home/pi/emonpi/emoncms-setup/emoncms-setup-sudoers /etc/sudoers.d/
    sudo chmod 0440 /etc/sudoers.d/emoncms-setup-sudoers
    
Install Emoncms module 

    ln -s /home/pi/emonpi/emoncms-setup /var/www/emoncms/Modules/setup
