*Not used on production emonpi, functionality is now built into main emonpi lcd script 

Python Script to look for high signal on GPIO 17 (pin 11) and then shutdown the Pi if so

on the emonPi GPIO 17 is connected to ATmega328 Dig5. 
When running default firmware emonPi sets Dig5/GPIO17 high when shutdown push button the the emonPi PCB is held for 5s. The push button is connected to ATmega328 dig 8

emonPi - Raspberry Pi Energy Monitoring 
Part of the OpenEnergyMonitor.org Project
