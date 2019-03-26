# How to run scripts manually

All commands assume current directory is emonpi/update

### Update Emoncms

    ./emoncms.sh HOME\_DIR IS\_EMONSD EMONCMS\_DIR
    ./emoncms.sh /home/pi 0 /var/www/emoncms
    
### Update EmonHub

    ./emonhub.sh HOME\_DIR
    ./emonhub.sh /home/pi
    
### Update Firmware: EmonPi

    ./emonpi.sh HOME\_DIR
    ./emonpi.sh /home/pi
    
### Update Firmware: rfm69pi

    ./rfm69pi.sh HOME\_DIR
    ./rfm69pi.sh /home/pi
    
### Via main.sh

    ./main.sh USERNAME TYPE FIRMWARE
    ./main.sh pi all emonpi
    ./main.sh pi emoncms
    ./main.sh pi emonhub
    ./main.sh pi firmware emonpi
