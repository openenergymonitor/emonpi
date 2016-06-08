## Compile and upload firmware using platformio

## Install patformio if needed

    sudo pip install platformio
    

## Install libs

Platformoio does not using the libraries in firmware/librarys instead we can install the libs direct from git via platform io lib manager.

jeeib:

    platformio lib install 252 --version="e70c9d9f4e"

dht22:

    platformio lib install 19 --version="09344416d2"

Dallas Temperature Control:

    platformio lib install 252 --version="3.7.7"
    
emonlib:

    platformio lib install 116 --version="7685720ab3"

LiquidCrystal_I2C:

    platformio lib install 576 --version="4bb48bd648"

OneWire:
    platformio lib install 1 --version="57c18c6de8"

## Compile
  
    $ platformio run

## Upload

    $ platformio run --target upload



