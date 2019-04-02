#! /bin/sh
source config.ini

echo "-------------------------------------------------------------"
echo "Install Emoncms Modules"
echo "-------------------------------------------------------------"
# Review default branch: e.g stable
cd $emoncms_www/Modules
for module in ${emoncms_modules[*]}; do
    echo "Installing module: $module"
    git clone https://github.com/emoncms/$module.git
done

# wifi module sudoers entry
sudo visudo -cf $usrdir/emonpi/wifi-sudoers && \
sudo cp $usrdir/emonpi/wifi-sudoers /etc/sudoers.d/
sudo chmod 0440 /etc/sudoers.d/wifi-sudoers
echo "wifi sudoers entry installed"

# Install emoncms modules that do not reside in /var/www/emoncms/Modules
if ![ -d $usrdir/modules ]; then
    # sudo mkdir /usr/emoncms
    # sudo chown $user /usr/emoncms
    mkdir $usrdir/modules
    # emoncms-sync.log is written to data folder
    # change to /var/log or use emoncms logger
    mkdir $usrdir/modules/data
fi

cd $usrdir
# usefulscripts
git clone https://github.com/emoncms/usefulscripts.git

cd $usrdir/modules
for module in ${emoncms_modules_usrdir[*]}; do
    echo "Installing module: $module"
    git clone https://github.com/emoncms/$module.git
    ln -s $usrdir/modules/$module/$module-module $emoncms_www/Modules/$module
done

# backup
# Rename emoncms module component to backup-module
git clone https://github.com/emoncms/backup.git
cd backup
git checkout multienv
cp default.emonpi.config.cfg config.cfg
sed -i "s/\/home\/pi\/backup/\/usr\/emoncms\/modules\/backup/" config.cfg
ln -s $usrdir/modules/backup/backup $emoncms_www/Modules/backup



