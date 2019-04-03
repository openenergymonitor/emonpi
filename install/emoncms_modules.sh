#!/bin/bash
source config.ini

echo "-------------------------------------------------------------"
echo "Install Emoncms Modules"
echo "-------------------------------------------------------------"
# Review default branch: e.g stable
cd $emoncms_www/Modules
for module in ${emoncms_modules[*]}; do
    if [ ! -d $module ]; then
        echo "- Installing module: $module"
        git clone https://github.com/emoncms/$module.git
    else
        echo "- Module $module already exists"
    fi
done

# wifi module sudoers entry
sudo visudo -cf $usrdir/emonpi/sudoers.d/wifi-sudoers && \
sudo cp $usrdir/emonpi/sudoers.d/wifi-sudoers /etc/sudoers.d/
sudo chmod 0440 /etc/sudoers.d/wifi-sudoers
echo "wifi sudoers entry installed"

# Install emoncms modules that do not reside in /var/www/emoncms/Modules
if [ ! -d $usrdir/modules ]; then
    # sudo mkdir /usr/emoncms
    # sudo chown $user /usr/emoncms
    mkdir $usrdir/modules
    # emoncms-sync.log is written to data folder
    # change to /var/log or use emoncms logger
    mkdir $usrdir/modules/data
fi

cd $usrdir/modules
for module in ${emoncms_modules_usrdir[*]}; do
    if [ ! -d $module ]; then
        echo "- Installing module: $module"
        git clone https://github.com/emoncms/$module.git
        # If module contains emoncms UI folder, symlink to $emoncms_www/Modules
        if [ -d $usrdir/modules/$module/$module-module ]; then
            echo "-- UI directory symlink"
            ln -s $usrdir/modules/$module/$module-module $emoncms_www/Modules/$module
        fi
        # If module contains service script: install
        if [ -f $usrdir/modules/$module/$module.service ]; then
            echo "-- installing service"
            servicepath=$usrdir/modules/$module/$module.service
            $usrdir/emonpi/update/install_emoncms_service.sh $servicepath $module      
        fi
    else
        echo "- Module $module already exists"
    fi
done

# backup module
if [ ! -d $usrdir/modules/backup ]; then
    # Rename emoncms module component to backup-module
    git clone https://github.com/emoncms/backup.git
    cd backup
    git checkout multienv
    cp default.emonpi.config.cfg config.cfg
    sed -i "s/\/home\/pi\/backup/\/usr\/emoncms\/modules\/backup/" config.cfg
    ln -s $usrdir/modules/backup/backup $emoncms_www/Modules/backup
else
    echo "- Module backup already exists"
fi
