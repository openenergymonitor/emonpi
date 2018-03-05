// Hardware configuration
//----------------------------emonPi V3 hard-wired connections---------------------------------------------------------------------------------------------------------------
const byte LEDpin = 13;             // emonPi LED - on when HIGH
const byte shutdown_switch_pin = 6; // Push-to-make - Low when pressed
const byte emonpi_GPIO_pin = 5;     // Connected to Pi GPIO 17, used to activate Pi Shutdown when HIGH
const byte oneWire_pin = 7;         // DS18B20 Data, RJ45 pin 4

// Only pins 2 or 3 can be used for interrupts:
const byte emonpi_pulse_pin = 2;    // default pulse count input

//http://openenergymonitor.org/emon/buildingblocks/calibration

struct Config {
        byte version;
        byte rf_enable;
        float Vcal;
        float Vrms;
        float Ical1;
        float Ical2;
        float phase_shift;
};

/* Ical = (2000 turns / burden)
   2000 / 33R = 60.6 for 50A max
   2000/ 150R = 13.3 for 10A max
*/
/* Change version to force rewrite of default to eeprom */
#define DEFAULT_CONFIG \
        .version = 2, \
        .rf_enable = 0, \
        .Vcal = 225.5, \
        .Vrms = 240, \
        .Ical1 = 60.6, \
        .Ical2 = 13.3, \
        .phase_shift = 1.7
