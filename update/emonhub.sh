#!/bin/bash
echo "-------------------------------------------------------------"
echo "emonHub update started"
echo "-------------------------------------------------------------"
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
usrdir=${DIR/\/emonpi\/update/}
echo "- usr directory: $usrdir"

if [ -d $usrdir/emonhub ]; then

    echo "git pull $usrdir/emonhub"
    cd $usrdir/emonhub
    git branch
    git status
    git pull
    
    echo "Creating emonhub logfile"
    if [ ! -d /var/log/emonhub ]; then
        sudo mkdir /var/log/emonhub
        sudo chown emonhub:emonhub /var/log/emonhub
    fi
    
    echo "Symlinking emonhub.conf to /etc/emonhub/emonhub.conf"
    if [ ! -d /etc/emonhub ]; then
        sudo mkdir /etc/emonhub
    fi
    sudo ln -sf $usrdir/data/emonhub.conf /etc/emonhub
    
    echo "Symlinking emonhub.py to /usr/local/bin"
    sudo ln -sf $usrdir/emonhub/src/emonhub.py /usr/local/bin

    service="emonhub"
    servicepath="$usrdir/emonhub/service/emonhub.service"
    $usrdir/emonpi/update/install_emoncms_service.sh $servicepath $service
    
    sudo systemctl daemon-reload
    sudo systemctl restart emonhub.service

    echo
    echo "Running emonhub automatic node addition script"
    $usrdir/emonhub/conf/nodes/emonpi_auto_add_nodes.sh $usrdir

else
    echo "EmonHub not found"
fi

