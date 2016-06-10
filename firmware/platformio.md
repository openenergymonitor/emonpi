## Compile and upload firmware using platformio

## Install patformio if needed.

See [platformio install quick start](http://docs.platformio.org/en/latest/installation.html#super-quick-mac-linux)

Recomended to use install script which may require sudo:

`python -c "$(curl -fsSL https://raw.githubusercontent.com/platformio/platformio/master/scripts/get-platformio.py)"`

or via python pipL

    sudo pip install platformio
    

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


## Compile
  
    $ platformio run -e emonpi_dev

## Upload

    $ platformio run -e emonpi_dev -t upload



