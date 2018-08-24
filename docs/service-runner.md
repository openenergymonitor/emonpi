# Service Runner

The service-runner facilitates the web page triggered updates. It was originally a bash script in an endless loop
that was also kept running using a crontab entry for the pi user triggered every minute.

A Python version of the service runner can be used instead which uses redis more efficiently, significantly reducing
processor load of the redis-server process.

To self-upgrade to the python service-runner, follow these instructions:
```
rpi-rw
git pull 
sudo pip install redis
sudo ln -s /home/pi/emonpi/service-runner.service /etc/systemd/system
sudo systemctl daemon-reload
sudo systemctl enable service-runner
sudo systemctl start service-runner
sudo systemctl status service-runner
```

You must also ensure you remove the entry from the pi user crontab:
```
rpi-rw
crontab -e
```
delete this line:
`* * * * * /home/pi/emonpi/service-runner >> /var/log/service-runner.log 2>&1`
Save and exit the editor, then check that it has been removed using:
```
crontab -l
```
