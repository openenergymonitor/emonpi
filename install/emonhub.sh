#!/bin/bash
source config.ini

echo "-------------------------------------------------------------"
echo "emonHub install"
echo "-------------------------------------------------------------"
cd $usrdir

if [ ! -d $usrdir/emonhub ]; then
    git clone https://github.com/openenergymonitor/emonhub.git
else 
    echo "- emonhub repository already installed"
    git pull
fi

if [ -f $usrdir/emonhub/install.sh ]; then
    $usrdir/emonhub/install.sh $emonSD_pi_env
else
    echo "ERROR: $usrdir/emonhub/install.sh script does not exist"
fi

# Sudoers entry (review!)
sudo visudo -cf $usrdir/emonpi/sudoers.d/emonhub-sudoers && \
sudo cp $usrdir/emonpi/sudoers.d/emonhub-sudoers /etc/sudoers.d/
sudo chmod 0440 /etc/sudoers.d/emonhub-sudoers
echo "emonhub service control sudoers entry installed"
