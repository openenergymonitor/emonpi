## Compile and upload firmware using platformio on emonPi 

### [See blog post](https://blog.openenergymonitor.org/2016/06/platformio/)

Code can eaisly be compiled and uploaded direcly on a RaspberryPi to the emonPi board. PlatformIO now supports `emonpi` board defintion with auto-reset on GPIO4 / pin 7 on upload. See commit:

https://github.com/platformio/platformio/commit/c5b5e80de4928cf91be59e675429b520e31d873a

Thanks @ivankravets  +1

## Install platformio on emonPi 

(if not already installed)

See [platformio install quick start](http://docs.platformio.org/en/latest/installation.html#super-quick-mac-linux)

Recomended to use install script which may require sudo:

`python -c "$(curl -fsSL https://raw.githubusercontent.com/platformio/platformio/master/scripts/get-platformio.py)"`

or via python pip
    
    sudo pip install -U urllib3
    sudo pip install -U platformio
    

**note update of urllib3 is needed on RaspberryPi**

## Compile
  
    $ platformio run 

## Compile Upload 

Compile and upload via /dev/ttyAMA0 using GPIO4 as emonpi autorest. **sudo required** ...for now

    $ sudo service emonhub stop
    $ sudo platformio run -t upload
    $ sudo service emonhub start 

Stopping emonhub is required to free up the serial port

## Install libs

**Note: installing libs is no longer requires since required libs are now defined in `platformio.ini`., in current version of platformio its not possible to specify a particualr version when definining in .ini. This will be fixed in platformio 3.0**

Platformoio does not using the libraries in firmware/librarys instead we can install the libs direct from git via platform io lib manager.

jeeib:

    platformio lib install 252 --version="e70c9d9f4e"

dht22:

    platformio lib install 19 --version="09344416d2"

Dallas Temperature Control:

    platformio lib install 54 --version="3.7.7"
    
emonlib:

    platformio lib install 116 --version="1.0"

LiquidCrystal_I2C:

    platformio lib install 576 --version="4bb48bd648"






