#!/bin/bash
source config.ini

# --------------------------------------------------------------------------------
# Install log2ram, so that logging is on RAM to reduce SD card wear.
# Logs are written to disk every hour or at shutdown
# --------------------------------------------------------------------------------
cd
git clone -b rsync_mods https://github.com/pb66/log2ram.git
cd log2ram
chmod +x install.sh && sudo ./install.sh
cd ..
rm -r log2ram

# --------------------------------------------------------------------------------
# Misc
# --------------------------------------------------------------------------------
# Review: provide configuration file for default password and hostname

# Set default SSH password:
printf "raspberry\n$ssh_password\n$ssh_password" | passwd

# Set hostname
sudo sed -i "s/raspberrypi/$hostname/g" /etc/hosts
printf $hostname | sudo tee /etc/hostname > /dev/null

# --------------------------------------------------------------------------------
# UFW firewall
# --------------------------------------------------------------------------------
# Review: reboot required before running:
sudo apt-get install -y ufw
# sudo ufw allow 80/tcp
# sudo ufw allow 443/tcp (optional, HTTPS not present)
# sudo ufw allow 22/tcp
# sudo ufw allow 1883/tcp #(optional, Mosquitto)
# sudo ufw enable

# Disable duplicate daemon.log logging to syslog
# This line is no longer needed if emonhub + emoncms handles its own logging
# sudo sed -i "s/*.*;auth,authpriv.none\t\t-\/var\/log\/syslog/*.*;auth,authpriv.none,daemon.none\t\t-\/var\/log\/syslog/" /etc/rsyslog.conf
# sudo systemctl restart rsyslog
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
