# How to run scripts manually

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
