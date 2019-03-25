#!/bin/bash

echo
echo "-------------------------------------------------------------"
echo "Emoncms update started"
echo "Emoncms update script V1.2.1 (6th Feb 2019)"
echo "-------------------------------------------------------------"
echo
homedir=$1 
emonSD_pi_env=$2
emoncms_dir=$3
emoncms_symlinked_modules="/usr/emoncms/modules"

if [ "$emonSD_pi_env" = "1" ]; then

    # Sudoers installation (provides sudo access to specific commands from emoncms)
    for sudoersfile in "emoncms-rebootbutton" "emoncms-filesystem" "emoncms-filesystem" "emoncms-setup-sudoers"; do
        if [ ! -f /etc/sudoers.d/$sudoersfile ]; then
            sudo visudo -cf $homedir/emonpi/sudoers.d/$sudoersfile && \
            sudo cp $homedir/emonpi/sudoers.d/$sudoersfile /etc/sudoers.d/
            sudo chmod 0440 /etc/sudoers.d/$sudoersfile
            echo
            echo "$sudoersfile sudoers entry installed"
        fi
    done
    
    # Setup user group to enable reading GPU temperature (pi only)
    sudo usermod -a -G video www-data
    
    # Add www-data to systemd group to enable read logfiles e.g emonhub 
    sudo usermod -a -G systemd-journal www-data
    groups www-data
fi

# -----------------------------------------------------------------
# Record current state of emoncms settings.php
# This needs to be run prior to emoncms git pull
# -----------------------------------------------------------------
echo
current_settings_md5="$($homedir/emonpi/./md5sum.py $emoncms_dir/settings.php)"
echo "current settings.php md5: $current_settings_md5"

current_default_settings_md5="$($homedir/emonpi/md5sum.py $emoncms_dir/default.emonpi.settings.php)"
echo "Default settings.php md5: $current_default_settings_md5"

if [ "$current_default_settings_md5" == "$current_settings_md5" ]; then
  echo "settings.php has NOT been user modifed"
  settings_unmodified=true
else
  echo "settings.php HAS been user modified"
  settings_unmodified=false
fi

# -----------------------------------------------------------------
# Pulling in latest Emoncms changes
# -----------------------------------------------------------------
echo
echo "Checking status of $emoncms_dir git repository"
cd $emoncms_dir
branch=$(git branch | grep \* | cut -d ' ' -f2)
echo "- git branch: $branch"
changes=$(git diff-index --quiet HEAD --)
if $changes; then
    echo "- no local changes"
    echo "- running: git pull origin $branch"
    echo
    git pull origin $branch
else
    echo "- changes"
fi

# -----------------------------------------------------------------
# check to see if user has modifed settings.php and if update is need. Auto apply of possible
# -----------------------------------------------------------------
echo
new_default_settings_md5="$($homedir/emonpi/md5sum.py $emoncms_dir/default.emonpi.settings.php)"
echo "NEW default settings.php md5: $new_default_settings_md5"

# check to see if there is an update waiting for settings.php
if [ "$new_default_settings_md5" != "$current_default_settings_md5" ]; then
  echo "Update required to settings.php..."
  if [ $settings_unmodified == true ]; then
    sudo cp $emoncms_dir/default.emonpi.settings.php $emoncms_dir/settings.php
    echo "settings.php autoupdated"
  else
    echo "**ERROR: unable to autoupdate settings.php since user changes are present, manual review required**"
  fi
else
  echo "settings.php not updated"
fi

# -----------------------------------------------------------------
echo
echo "-------------------------------------------------------------"
echo "Updating Emoncms Modules"
echo "-------------------------------------------------------------"
echo

# Update modules installed directly in the Modules folder
for M in $emoncms_dir/Modules/*; do
  if [ -d "$M/.git" ]; then
    echo "------------------------------------------"
    echo "Updating $M module"
    echo "------------------------------------------"
    
    branch=$(git -C $M branch | grep \* | cut -d ' ' -f2)
    echo "- git branch: $branch"
    tags=$(git -C $M describe --tags)
    echo "- git tags: $tags"
    
    changes=$(git -C $M diff-index HEAD --)
    if [ "$changes" = "" ]; then
        echo "- no local changes"
        echo "- running: git pull origin $branch"
        echo
        git -C $M pull origin $branch
    else
        echo "- git status:"
        echo
        git -C $M status
        echo
    fi
    
    echo
  fi
done

# Update modules installed in the home/user folder
for module in "postprocess" "sync" "backup"; do

  echo "------------------------------------------"
  echo "Updating $module module"
  echo "------------------------------------------"
  if [ -d $emoncms_symlinked_modules/$module ]; then
    cd $emoncms_symlinked_modules/$module
    branch=$(git branch | grep \* | cut -d ' ' -f2)
    echo "- git branch: $branch"
    tags=$(git describe --tags)
    echo "- git tags: $tags"
    changes=$(git diff-index HEAD --)
    if [ "$changes" = "" ]; then
        echo "- no local changes"
        echo "- running: git pull origin $branch"
        echo
        git pull origin $branch
        # ln -sf $homedir/$module/$module-module $emoncms_dir/Modules/$module
    else
        echo "- git status:"
        echo
        git status
        echo
    fi
  else
    echo "$module module does not exist"
  fi
  echo
done

exit 0
#########################################################################################
# Automatic installation of new modules if they dont already exist
echo "=========================================="
echo "AUTO INSTALL EMONCMS MODULES"
echo "=========================================="

# Direct installation in Modules folder
for module in "graph" "device"; do
  if ! [ -d $emoncms_dir/Modules/$module ]; then
    echo "- Installing Emoncms $module module https://github.com/emoncms/$module"
    cd $emoncms_dir/Modules
    git clone https://github.com/emoncms/$module
    cd $emoncms_dir/Modules/$module
    if [ `git branch --list stable` ]; then
       echo "-- git checkout stable"
       git checkout stable
    fi
  else
    echo "- $module module already installed"
  fi
done

# Direct installation in Modules folder
for module in "postprocess" "sync" "backup"; do
  if ! [ -d $homedir/$module ]; then
    echo
    echo "- Installing Emoncms $module module https://github.com/emoncms/$module"
    cd $homedir
    git clone https://github.com/emoncms/$module
    ln -s $homedir/$module/$module-module $emoncms_dir/Modules
    cd $homedir/$module/$module-module
    if [ `git branch --list stable` ]; then
       echo "-- git checkout stable"
       git checkout stable
    fi
    echo
  else
    echo "- $module module already installed"
  fi
done
echo
#########################################################################################


if [ "$emonSD_pi_env" = "1" ]; then 
  # Install emoncms-setup module (symlink from emonpi repo)
  if [ -d $homedir/emonpi/emoncms-setup ] && [ ! -d $emoncms_dir/Modules/setup ]; then
    echo "Installing emoncms/emonPi setup module: symlink from ~/emonpi/emoncms-setup"
    ln -s $homedir/emonpi/emoncms-setup $emoncms_dir/Modules/setup
  else
    if [ ! -d $homedir/emonpi/emoncms-setup ]; then
      echo "Cannot find emoncms-setup module, please update ~/emonpi repo"
    fi
  fi
  echo
fi


echo "=========================================="
echo "SERVICES"
echo "=========================================="
# Removal of services
# 6th Feb 2019: mqtt_input renamed to emoncms_mqtt
for service in "mqtt_input"; do
  /home/pi/emonpi/update/remove_emoncms_service.sh $service
done
# testing on emonpi the above attempts to stop did not work?
# make sure its dead!!
sudo pkill -f phpmqtt_input.php

# Installation or correction of services
for service in "emoncms_mqtt" "feedwriter" "service-runner"; do
  /home/pi/emonpi/update/install_emoncms_service.sh $emoncms_dir $service
done
echo "------------------------------------------"

################################################################################################
## Don't overwrite settings unless required (un comment if required)
# echo "Copy new default.emonpi.settings.php to settings.php & make backup old.emonpi.settings.php"
# sudo cp $emoncms_dir/settings.php $emoncms_dir/old.settings.php
# sudo cp $emoncms_dir/default.emonpi.settings.php $emoncms_dir/settings.php

#if [ "$emonSD_pi_env" = "1" ]; then 
echo "Update Emoncms database"
php $homedir/emonpi/emoncmsdbupdate.php
echo
#fi

echo "Restarting Services..."
if [ -d /lib/systemd/system ]; then
  sudo systemctl daemon-reload
fi

for service in "feedwriter" "mqtt_input" "emoncms_mqtt" "emoncms-nodes-service" "emonhub" "openhab"; do
  if [ -f /lib/systemd/system/$service.service ]; then
    echo "- sudo systemctl restart $service.service"
    sudo systemctl restart $service.service
    state=$(systemctl show $service | grep ActiveState)
    echo "--- $state ---"
  elif [ -f /etc/init.d/$service ]; then
    echo "- sudo /etc/init.d/$service restart"
    sudo /etc/init.d/$service restart
    sudo /etc/init.d/$service status
  fi
done

###################################################################################
# Intalll log rotate config
###################################################################################
echo "Installing emoncms logrotate..."
echo

# Remove already roated old log files to free up space incase /var/log is full
if ls /var/log/syslog.* > /dev/null 2>&1; then
  sudo rm /var/log/syslog.*
fi

if ls /var/log/*.log.* > /dev/null 2>&1; then
  sudo rm /var/log/*.log.*
fi

# Install emonPi log rotate config
sudo /var/www/emoncms/scripts/logger/install.sh
# Run log roate manually
echo "Running logrotate..."
sudo /usr/sbin/logrotate -v -s /var/log/logrotate/logrotate.status /etc/logrotate.conf > /dev/null 2>&1

# Reload rather than restart apache so we dont loose the interface 
sudo service apache2 reload 
echo
if [ "$emonSD_pi_env" = "1" ]; then 
  echo "set log rotate config owner to root"
  sudo chown root:root /etc/logrotate.conf
fi
echo
echo "------------------------------------------"
echo "Emoncms update script complete"
echo "------------------------------------------"
