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

    # PUT ON HOLD
    #service="emonhub"
    #servicepath="$usrdir/emonhub/service/emonhub.service"
    #$usrdir/emonpi/update/install_emoncms_service.sh $servicepath $service

    echo
    echo "Running emonhub automatic node addition script"
    $usrdir/emonhub/conf/nodes/emonpi_auto_add_nodes.sh $usrdir

else
    echo "EmonHub not found"
fi

