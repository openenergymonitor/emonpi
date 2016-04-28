
# Arduino libraries for emonPi firmware

|  Name | Date  | Version  | Source  | Commit |
|---|---|---|---|---|
|  emonLib |  Jan 5th 2015 | N/A  | [source](https://github.com/openenergymonitor/emonlib)  | [7685720](https://github.com/openenergymonitor/EmonLib/commit/7685720ab391b14edb218151c1d5d3ebc1fd1ec1)  |
|  JeeLib |  8th April 2016 | N/A  |  [source](https://github.com/jcw/jeelib) |  [68f6e42 ](https://github.com/jcw/jeelib/commit/6f1af25695a51910d2bb8ca0e796a7edda028848e) |
|  LiquidCrystal_I2C | 29th Nov 2015  | 1.1.2  | [source](https://github.com/marcoschwartz/LiquidCrystal_I2C)  | [6ab83c7](https://github.com/marcoschwartz/LiquidCrystal_I2C/commit/9a4e33e6cdaca805d70e220897d0b59446d52adf)  |
| OneWire  | 4th March 2016  | N/A  | [source](https://github.com/PaulStoffregen/OneWire)  | [ 57c18c6](https://github.com/PaulStoffregen/OneWire/commit/57c18c6de80c13429275f70875c7c341f1719201)
| Dallas Temperature |  25th Jan 2016 | V3.7.6   | [source](https://github.com/milesburton/Arduino-Temperature-Control-Library)  | [7e9a2b7](https://github.com/milesburton/Arduino-Temperature-Control-Library/commit/7e9a2b710ae713d0686c3f921c1bbe0b4ebd23fb) |
| Arduino Wire |  N/A | V1.0   | included in Arduino IDE  | N/A  |

# Installing emonPi Arduino Libraries 

## Git Sub Modules 

The Arduino librarys used by the emonPi firmware uses [git submodules](https://git-scm.com/book/en/v2/Git-Tools-Submodules) to link the github librarys (at a specific version) to the emonPi firmware. 

Using sub modules has the advantage of keeping the link between the origional lib repository while at the same time allowing us to specify the state of each library. We have implemented this to ensure when a user clones the emonPi fimware and attempts to compile we can be sure that the libs used are **exactly** the same as the libs used when we compiled the firmware. 

After cloning or checking out the repo on further step is needed to pull in the sub modules:

	$ git submodule init
	$ git submodule update

## Arduino IDE setup 

**Tested with Arduino 1.6.8 64Bit**

To tell Arduino IDE to use the libraries in `emonpi/firmware/libraries` we need to set the **Arduino IDE Sketchbook location** to `*<localpath>*/emonpi/firmware/libraries` then restart the Arduino IDE.

On compiling check that Arduino is used the correct library, turn on *preferances>Show verbose output during compilation* and see log message at the beginning of compilation showing lib path. You might need to remove any lib you have sharing the same name in your Arduino sketchbook folder. 
