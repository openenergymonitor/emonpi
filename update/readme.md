# How to run scripts manually

Following works well with the suggested /usr/emoncms location. Assuming repo is installed in that directory e.g /usr/emoncms/emonpi. It also works fine with the /home/pi/emonpi path. Path is automatically detected.

---

Current directory is emonpi/update

### Update Emoncms

    ./emoncms.sh IS_EMONSD
    ./emoncms.sh 0
    
### Update EmonHub

    ./emonhub.sh
    
### Update Firmware: EmonPi

    ./emonpi.sh
    
### Update Firmware: rfm69pi

    ./rfm69pi.sh
    
### Via main.sh

    ./main.sh UPDATE_TYPE FIRMWARE IMAGE_NAME
    ./main.sh all emonpi emonSD-30Oct18
    ./main.sh emoncms emonSD-30Oct18
    ./main.sh emonhub emonSD-30Oct18
    ./main.sh firmware emonpi emonSD-30Oct18
    
### Via service-runner-update.sh

Current directory is emonpi.

This script is triggered from emoncms via service-runner.

    ./service-runner-update.sh UPDATE_TYPE FIRMWARE
    ./service-runner-update.sh all emonpi
