#!/bin/bash
# Run with sudo ./install

# if using minibian https://minibianpi.wordpress.com/2015/11/12/minibian-jessie-2015-11-12-is-out/
minibian = false

echo "update"
sudo apt-get update -y
sudo apt-get upgrade -y



# extra packeges needed if staring from lightweight minibam
# wireless-regdb iw crda are used to fix "Calling CRDA to update world regulatory domain" error messages due to missing packages in Jessie https://www.raspberrypi.org/forums/viewtopic.php?f=28&t=122067
if [minibian = true]; then
	sudo apt-get install -y iptables wpasupplicant strace nano dpkg logrotate wireless-regdb iw crda firmware-brcm80211 firmware-ralink apt-utils
	sudo apt-get remove -y exim4-base
else
	echo "deleting unused software"
	sudo apt-get purge -y wolfram-engine minecraft-pi sonic-pi scratch
	sudo apt-get autoremove -y
	sudo apt-get clean
	sudo apt-get install -y logrotate
fi

echo "Set hostname to emonpi"

if grep -Fxq "raspberrypi" /etc/hosts ; then
	sudo sed -i 's/raspberrypi/emonpi/' /etc/hosts
	sudo sed -i 's/raspberrypi/emonpi/' /etc/hostname
fi

if grep -Fxq "minibian" /etc/hosts ; then
	sudo sed -i 's/minibian/emonpi/' /etc/hosts
	sudo sed -i 's/minibian/emonpi/' /etc/hostname
fi	
	
	



echo "enable serial uploads with avrdue and autoreset & install minicom & enable use of serial port"
git clone https://github.com/openenergymonitor/avrdude-rpi.git ~/avrdude-rpi && ~/avrdude-rpi/install

#echo "install ino Arduino compile tool"
#sudo apt-get install arduino -y
#sudo apt-get install python-pip -y
#pip install ino

echo "disable the raspi-config auto run"
sudo rm /etc/profile.d/raspi-config.sh

# Only used when emonPi is used without LCD, run either LCD service or shutdwn service. LCD script also handles shutdown 
# echo "install shutdown service"
# sudo /home/pi/emonpi/shutdownpi/install

echo "install LCD"
sudo /home/pi/emonpi/lcd/install

echo "move /etc/resolv.conf to RW data partition to allow updating of DNS nameservers"
# create sim link of resolv.conf and resolv.conf.dhclient-new in RW /data to /etc
cp /etc/resolv.conf /home/pi/data/
sudo rm /etc/resolv.conf 
sudo ln -s /home/pi/data/resolv.conf /etc/resolv.conf

# Create resolv.conf.dhclient-new file and symlink to /etc
touch /home/pi/data/resolv.conf.dhclient-new
sudo rm /etc/resolv.conf.dhclient-new
sudo ln -s /home/pi/data/resolv.conf.dhclient-new /etc/resolv.conf.dhclient-new

# make backup of  dhclient-script only if backup has not been made before!
if [ ! -f /sbin/dhclient-script_original ]; then
    sudo cp /sbin/dhclient-script /sbin/dhclient-script_original
fi
# use new dhclient script 
sudo cp /home/pi/emonpi/dhclient-script /sbin/dhclient-script

# if using minibian
if [minibian = true]; then
	sudo cp /home/pi/emonpi/dhclient-script_minibam /sbin/dhclient-script
fi
sudo chmod a+x /sbin/dhclient-script

# Best do these steps manually: https://github.com/openenergymonitor/emonpi/blob/master/imagebuild.md
#echo "Move log directory to temporary partition"
#sudo cp /etc/default/rcS /etc/default/rcS.orig
#sudo sh -c "echo 'RAMTMP=yes' >> /etc/default/rcS"

#echo "setup /etc/fstab to make file system read-only be default"
#sudo sh -c "echo 'RAMTMP=yes' >> /etc/default/rcS"
#sudo mv /etc/fstab /etc/fstab.orig
#sudo sh -c "echo 'tmpfs           /tmp            tmpfs   nodev,nosuid,size=30M,mode=1777       0    0' >> /etc/fstab"
#sudo sh -c "echo 'tmpfs           /var/log        tmpfs   nodev,nosuid,size=30M,mode=1777       0    0' >> /etc/fstab"
#sudo sh -c "echo 'tmpfs           /var/lib/dhcp   tmpfs   nodev,nosuid,size=1M,mode=1777        0    0' >> /etc/fstab"
#sudo sh -c "echo 'proc            /proc           proc    defaults                              0    0' >> /etc/fstab"
#sudo sh -c "echo '/dev/mmcblk0p1  /boot           vfat    defaults                              0    2' >> /etc/fstab"
#sudo sh -c "echo '/dev/mmcblk0p2  /               ext4    defaults,ro,noatime,errors=remount-ro 0    1' >> /etc/fstab"
#sudo sh -c "echo '/dev/mmcblk0p3  /home/pi/data   ext2    defaults,rw,noatime                   0    2' >> /etc/fstab"
#sudo sh -c "echo ' ' >> /etc/fstab"
#sudo mv /etc/mtab /etc/mtab.orig
#sudo ln -s /proc/self/mounts /etc/mtab

echo "install fstab"
sudo rm /etc/fstab 
sudo ln -s /home/pi/emonpi/fstab /etc/fstab

echo "insall rc.local startup"
sudo rm /etc/rc.local 
sudo ln -s /home/pi/emonpi/rc.local /etc/rc.local
sudo chmod a+x /etc/rc.local

echo "Install Read-only NTP time fix"
git clone https://github.com/emonhub/ntp-backup.git ~/ntp-backup && ~/ntp-backup/install

# Add startup message
if ! grep -Fxq "The file system is in Read Only (RO) mode" /etc/motd ; then
	sudo echo " "
	sudo echo -e "\nThe file system is in Read Only (RO) mode. If you need to make changes, use the command rpi-rw to put the file system in Read Write (RW) mode. Use rpi-ro to return to RO mode. The /home/pi/data directory is always in RW mode \n" >> /etc/motd
	echo 'welcome message added to /etc/motd'
else
	echo 'welcome message already added to /etc/motd'
fi

# http://openenergymonitor.org/emon/node/11803
echo "Setup Logrotate" 
sudo rm /etc/logrotate.conf
sudo ln -s /home/pi/emonpi/logrotate.conf /etc/logrotate.conf
sudo chown root /etc/logrotate.conf
sudo ln -s /home/pi/emonpi/logrotate /etc/cron.hourly/logrotate
sudo rm /etc/cron.daily/logrotate
sudo touch /etc/cron.daily/logrotate


echo "Add wifi-check script to /user/local/bin"
sudo cp /home/pi/emonpi/wifi-check /usr/local/bin/wifi-check
sudo chmod +x /usr/local/bin/wifi-check

echo "Install wifi-check script in root crontab entry, every 5 min"
# Add root crontab entry to run wifi check script ever 5min
sudo crontab -l > mycron #write out current root crontab
# Check crontab entry does not already exist, if not add new entry
if ! grep -Fxq "*/5 * * * * /usr/local/bin/wifi-check > /var/log/wificheck.log 2>&1" mycron ; then
        echo " */5 * * * * /usr/local/bin/wifi-check > /var/log/wificheck.log 2>&1" >> mycron  #echo new cron into cronfile
        sudo crontab mycron #install new cron file
fi
sudo rm mycron

