#!/bin/sh

# Script that searches in /var/log for subdirectories and updates
# tmpfiles.d config file to ensure that all directories are recreated
# on boot

# This script could be called:
#   * Manually after user has installed their own packages
#   * During shutdown via systemd. see:
#     https://superuser.com/questions/1016827/how-do-i-run-a-script-before-everything-else-on-shutdown-with-systemd
#   * Automatically after installing packages via apt-get. see:
#     https://www.cyberciti.biz/faq/debian-ubuntu-linux-hook-a-script-command-to-apt-get-upgrade-command/

# Requires /var/log and /usr/lib/tmpfiles.d
if [ ! -d /var/log ]; then
  echo "/var/log does not exist"
  exit 1;
fi
if [ ! -d /usr/lib/tmpfiles.d ]; then
  echo "/usr/lib/tmpfiles.d does not exist"
  exit 1;
fi

STATIC_CONF=/usr/lib/tmpfiles.d/emonpi.conf
DYNAMIC_CONF=/usr/lib/tmpfiles.d/emonpi-dynamic.conf

DYNAMIC_HEADER="# This file is part of emonpi
#
# Since /var/log is mounted as tmpfs, the contents do not persist across reboots.
#
# This file contains entries for directories in /var/log that have been discovered
# dynamically at runtime. For instance directories created by extra packages
# that have been installed by the user
#
# The update-tmpfiles-config.sh script will search /var/log and append entries
# to this file for any directories that exist that are not already managed by tmpfiles

# See tmpfiles.d(5) for details

# Type Path    Mode UID  GID  Age Argument
"

# Make sure config files exist
# TODO: copy static file from /home/pi/emonpi/???
[ -f ${STATIC_CONF}  ] || /usr/bin/touch ${STATIC_CONF}

[ -f ${DYNAMIC_CONF} ] || echo "${DYNAMIC_HEADER}" > ${DYNAMIC_CONF}

# Find any unknown directories that have been created in /var/log, and add
# dynamic entries for them
for d in `find /var/log -mindepth 1 -type d`; do
  /bin/grep -q $d ${STATIC_CONF} ${DYNAMIC_CONF}
  if [ $? -ne 0 ]; then
    echo "Found new log dir in tmpfs mounted /var/log: $d"
    /usr/bin/stat -c "d %n %#a %U %G" $d >>${DYNAMIC_CONF}
  fi
done
