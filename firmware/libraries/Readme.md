
# Arduino libraries for emonPi firmware

|  Name | Date  | Version  | Source  | Commit |
|---|---|---|---|---|
|  emonLib |  Jan 5th 2015 | N/A  | [source](https://github.com/openenergymonitor/emonlib)  | [7685720](https://github.com/openenergymonitor/EmonLib/commit/7685720ab391b14edb218151c1d5d3ebc1fd1ec1)  |
|  JeeLib |  10th Sep 2015 | N/A  |  [source](https://github.com/jcw/jeelib) |  [f097c00](https://github.com/jcw/jeelib/commit/f097c0039c926881d80a74bec7a7aa020de610ee) |
|  LiquidCrystal | 7th Feb 2016  | N/A  | [source](https://github.com/openenergymonitor/LiquidCrystal_I2C1602V1)  | [6ab83c7](https://github.com/openenergymonitor/LiquidCrystal_I2C1602V1/commit/6ab83c7b8cf2360c4c82631d158b083bf95cb6d7)  |
| OneWire  | 4th March 2016  | N/A  | [source](https://github.com/PaulStoffregen/OneWire)  | N/A  | [ 57c18c6](https://github.com/PaulStoffregen/OneWire/commit/57c18c6de80c13429275f70875c7c341f1719201)
| Dallas Temperature |  6th Dec 2011 | V3.7.2   | [source](http://download.milesburton.com/Arduino/MaximTemperature/DallasTemperature_LATEST.zip)  | N/A  |

# Installing emonPi Arduino Libraries 

## Git Sub Modules 

The Arduino librarys used by the emonPi firmware uses [git submodules](https://git-scm.com/book/en/v2/Git-Tools-Submodules) to link the github librarys (at a specific version) to the emonPi firmware. 

Using usb modules has the advantage of keeping the link between the origional lib repository while at the same time allowing us to specify the state of each library. We have implemented this to ensure when a user clones the emonPi fimware and attempts to compile we can be sure that the libs used are **exactly** the same as the libs used when we compiled the firmware. 

After cloning or checking out the repo on further step is needed to pull in the sub modules:

	$ git submodule init
	$ git submodule update

## Arduino IDE setup 

**Tested with Arduino 1.6.8 64Bit**

To tell Arduino IDE to use the libraries in `emonpi/firmware/libraries` we need to set the **Arduino IDE Sketchbook location** to `*<localpath>*/emonpi/firmware/libraries` then restart the Arduino IDE.

On compiling check that Arduino is used the correct library, turn on *preferances>Show verbose output during compilation* and see log message at the beginning of compilation showing lib path. You might need to remove any lib you have sharing the same name in your Arduino sketchbook folder. 
	
