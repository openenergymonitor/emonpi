
### DISCLAIMER
### This is an example Makefile and it MUST be configured to suit your needs.
### For detailed explanations about all of the available options, please refer
### to https://github.com/sudar/Arduino-Makefile/blob/master/arduino-mk-vars.md
### Original project where this Makefile comes from: https://github.com/WeAreLeka/Bare-Arduino-Project

### PROJECT_DIR
### This is the path to where you have created/cloned your project
PROJECT_DIR       =$(HOME)/Downloads/dev/emonpi/Atmega328/emonPi_RFM69CW_RF12Demo_DiscreteSampling

### ARDMK_DIR
### Path to the Arduino-Makefile directory.
#ARDMK_DIR         = $(PROJECT_DIR)/Arduino-Makefile
ARDMK_DIR         = $(HOME)/Downloads/tools/Arduino-Makefile

### ARDUINO_DIR
### Path to the Arduino application and resources directory.
ARDUINO_DIR       = $(HOME)/Downloads/tools/arduino-1.6.7

### USER_LIB_PATH
### Path to where the your project's libraries are stored.
# USER_LIB_PATH    :=  $(PROJECT_DIR)/lib
USER_LIB_PATH      := $(ARDUINO_DIR)/libraries

### BOARD_TAG
### It must be set to the board you are currently using. (i.e uno, mega2560, etc.)
#BOARD_TAG         = uno

ARCHITECTURE = avr
BOARD_TAG = uno
BOARD_SUB = atmega328p

### MONITOR_BAUDRATE
### It must be set to Serial baudrate value you are using.
MONITOR_BAUDRATE  = 9600

### AVR_TOOLS_DIR
### Path to the AVR tools directory such as avr-gcc, avr-g++, etc.
AVR_TOOLS_DIR     = /usr

### AVRDUDE
### Path to avrdude directory.
AVRDUDE          = $(AVR_TOOLS_DIR)/bin/avrdude

### CFLAGS_STD
### Set the C standard to be used during compilation. Documentation (https://github.com/WeAreLeka/Arduino-Makefile/blob/std-flags/arduino-mk-vars.md#cflags_std)
CFLAGS_STD        = -std=gnu11

### CXXFLAGS_STD
### Set the C++ standard to be used during compilation. Documentation (https://github.com/WeAreLeka/Arduino-Makefile/blob/std-flags/arduino-mk-vars.md#cxxflags_std)
CXXFLAGS_STD      = -std=gnu++11

### CXXFLAGS
### Flags you might want to set for debugging purpose. Comment to stop.
CXXFLAGS         += -pedantic -Wall -Wextra

### MONITOR_PORT
### The port your board is connected to. Using an '*' tries all the ports and finds the right one.
MONITOR_PORT      = /dev/tty.usbmodem*

### CURRENT_DIR
### Do not touch - used for binaries path
CURRENT_DIR       = $(shell basename $(CURDIR))

### OBJDIR
### This is where you put the binaries you just compile using 'make'
#OBJDIR            = $(PROJECT_DIR)/bin/$(BOARD_TAG)/$(CURRENT_DIR)
OBJDIR            =  $(ARDUINO_DIR)/bin

### Do not touch - the path to Arduino.mk, inside the ARDMK_DIR
include $(ARDMK_DIR)/Arduino.mk
