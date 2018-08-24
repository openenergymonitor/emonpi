# Install service runner 

The service runner is used to trigger scrips (e.g update / backup) from emoncms, it needs to be running continuously. 

Service runner is  bridge between the web application and update bash scripts.

The process is as follows:

1. Web application triggers an update by setting a flag in redis
2. Service runner continuously polls redis for an update flag
3. Service runner start the update and pipes the log back to the web application

## Install python systemd service running 

```
sudo pip install redis
sudo ln -s /home/pi/emonpi/service-runner.service /etc/systemd/system
sudo systemctl daemon-reload
sudo systemctl enable service-runner
sudo systemctl start service-runner
sudo systemctl status service-runner
```

Tested on emonPi running Raspiben Stretch

Prior to September 2018 the service runner was ran as a bash script triggered by cron. Thanks to @greeebs for re-writing using Python and systemd see https://github.com/openenergymonitor/emonpi/pull/65. The python service is more efficient since a constant connetion to redis can be kept open
