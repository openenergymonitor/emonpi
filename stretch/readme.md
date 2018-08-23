# Raspbian Stretch

This folder contains config files symlinked into the latest emonSD pre-build raspberry pi SD card image based on Raspbian Stretch. 

- Compatiable with RasPi 3B+ 
- Based on Raspbian Stretch Lite 
- **Does not have read-only root FS** - Unlike older emonSD card images, the Stretch image does not have a read-only root partition. After extensive testing and file system monitoring it was determind that we could achieve SD card low write by only mounting /var/log in temp fs and keeping the root partition in RW all the time. This has the advantage of making setup, config and maintenance much easier since the image is closer to the stock raspbian image.
- The folder `~/data``is still mounted on a seperate partition with a low write file-system
