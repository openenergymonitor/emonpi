#!/bin/bash
echo "-------------------------------------------------------------"
echo "emonHub update started"
echo "-------------------------------------------------------------"
echo
homedir=$1

echo "git pull $homedir/emonhub"
cd $homedir/emonhub
git branch
git status
git pull

service="emonhub"
servicepath="$homedir/emonhub/service/emonhub.service"
$homedir/emonpi/update/install_emoncms_service.sh $servicepath $service

echo
echo "Running emonhub automatic node addition script"
$homedir/emonhub/conf/nodes/emonpi_auto_add_nodes.sh $homedir

