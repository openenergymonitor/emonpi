#! /bin/sh
# auto detect install location (e.g /usr/emoncms or /home/pi)
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
usrdir=${DIR/\/emonpi\/install/}

emoncms_www=/var/www/emoncms

echo "-------------------------------------------------------------"
echo "Install Emoncms Modules"
echo "-------------------------------------------------------------"
# Review default branch: e.g stable
cd /var/www/emoncms/Modules
git clone https://github.com/emoncms/config.git
git clone https://github.com/emoncms/graph.git
git clone https://github.com/emoncms/dashboard.git
git clone https://github.com/emoncms/device.git
git clone https://github.com/emoncms/app.git
git clone https://github.com/emoncms/wifi.git

# wifi module sudoers entry
sudo visudo -cf $usrdir/emonpi/wifi-sudoers && \
sudo cp $usrdir/emonpi/wifi-sudoers /etc/sudoers.d/
sudo chmod 0440 /etc/sudoers.d/wifi-sudoers
echo "wifi sudoers entry installed"

# Install emoncms modules that do not reside in /var/www/emoncms/Modules
if ![ -d $usrdir/modules ]; then
    # sudo mkdir /usr/emoncms
    # sudo chown $USER /usr/emoncms
    mkdir $usrdir/modules
    # emoncms-sync.log is written to data folder
    # change to /var/log or use emoncms logger
    mkdir $usrdir/modules/data
fi

cd $usrdir
# usefulscripts
git clone https://github.com/emoncms/usefulscripts.git

cd $usrdir/modules
# postprocess
git clone https://github.com/emoncms/postprocess.git
ln -s $usrdir/modules/postprocess/postprocess-module $emoncms_www/Modules/postprocess
# demandshaper
git clone https://github.com/emoncms/demandshaper.git
ln -s $usrdir/modules/demandshaper/demandshaper-module $emoncms_www/Modules/demandshaper
# sync
git clone https://github.com/emoncms/sync.git
ln -s $usrdir/modules/sync/sync-module $emoncms_www/Modules/sync
# backup
# Rename emoncms module component to backup-module
git clone https://github.com/emoncms/backup.git
cd backup
git checkout multienv
cp default.emonpi.config.cfg config.cfg
sed -i "s/\/home\/pi\/backup/\/usr\/emoncms\/modules\/backup/" config.cfg
ln -s $usrdir/modules/backup/backup $emoncms_www/Modules/backup



