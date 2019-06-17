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
    
    # emonhub source
    sudo ln -sf $usrdir/emonhub/src /usr/local/bin/emonhub
    
    # emonhub.conf
    sudo ln -sf $usrdir/data/emonhub.conf /etc/emonhub/emonhub.conf

    # remove emonhub from old symlinked location
    if [ -L /usr/share/emonhub ]; then
        sudo rm /usr/share/emonhub
    fi

    # remove emonhub.env
    if [ -f /etc/emonhub/emonhub.env ]; then
        sudo rm /etc/emonhub/emonhub.env
    fi

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

