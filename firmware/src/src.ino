/*

  emonPi  Discrete Sampling

  If AC-AC adapter is detected assume emonPi is also powered from adapter (jumper shorted) and take Real Power Readings and disable sleep mode to keep load on power supply constant
  If AC-AC addapter is not detected assume powering from battereis / USB 5V AC sample is not present so take Apparent Power Readings and enable sleep mode

  Transmit values via RFM69CW radio

   ------------------------------------------
  Part of the openenergymonitor.org project

  Authors: Glyn Hudson & Trystan Lea
  Builds upon JCW JeeLabs RF12 library and Arduino

  Licence: GNU GPL V3

*/

/*Recommended node ID allocation
------------------------------------------------------------------------------------------------------------
-ID-	-Node Type-
0	- Special allocation in JeeLib RFM12 driver - reserved for OOK use
1-4     - Control nodes
5-10	- Energy monitoring nodes
11-14	--Un-assigned --
15-16	- Base Station & logging nodes
17-30	- Environmental sensing nodes (temperature humidity etc.)
31	- Special allocation in JeeLib RFM12 driver - Node31 can communicate with nodes on any network group
-------------------------------------------------------b------------------------------------------------------


Change Log:
https://github.com/openenergymonitor/emonpi/blob/master/Atmega328/emonPi_RFM69CW_RF12Demo_DiscreteSampling/compiled/CHANGE%20LOG.md

emonhub.conf node decoder:

[[5]]
    nodename = emonPi
    firmware = emonPi_RFM69CW_RF12Demo_DiscreteSampling.ino
    hardware = emonpi
    [[[rx]]]
        names = power1,power2,power1_plus_power2,Vrms,T1,T2,T3,T4,T5,T6,pulseCount
        datacodes = h, h, h, h, h, h, h, h, h, h, L
        scales = 1,1,1,0.01,0.1,0.1,0.1,0.1,0.1,0.1,1
        units = W,W,W,V,C,C,C,C,C,C,p

*/

#define emonTxV3                                                      // Tell emonLib this is the emonPi V3 - don't read Vcc assume Vcc = 3.3V as is always the case on emonPi eliates bandgap error and need for calibration http://harizanov.com/2013/09/thoughts-on-avr-adc-accuracy/
#define RF69_COMPAT 1                                                 // Set to 1 if using RFM69CW or 0 is using RFM12B

#include <JeeLib.h>                                                   // https://github.com/openenergymonitor/jeelib
#include <avr/pgmspace.h>
#include <util/parity.h>
ISR(WDT_vect) { Sleepy::watchdogEvent(); }                            // Attached JeeLib sleep function to Atmega328 watchdog -enables MCU to be put into sleep mode inbetween readings to reduce power consumption

#include "EmonLib.h"                                                  // Include EmonLib energy monitoring library https://github.com/openenergymonitor/EmonLib
EnergyMonitor ct1, ct2;

#include <OneWire.h>                                                  // http://www.pjrc.com/teensy/td_libs_OneWire.html
#include <DallasTemperature.h>                                        // http://download.milesburton.com/Arduino/MaximTemperature/DallasTemperature_LATEST.zip
#include <DS2438.h>                                                   // https://github.com/jbechter/arduino-onewire-DS2438


#include <Wire.h>                                                     // Arduino I2C library
#include <LiquidCrystal_I2C.h>                                        // https://github.com/openenergymonitor/LiquidCrystal_I2C

int i2c_lcd_address[2]={0x27, 0x3f};                                  // I2C addresses to test for I2C LCD device
int current_lcd_i2c_addr;                                                  // Used to store current I2C address as found by i2_lcd_detect()
// LiquidCrystal_I2C lcd(0x27,16,2);                                  // Placeholder
LiquidCrystal_I2C lcd(0,0,0);


//----------------------------emonPi Firmware Version---------------------------------------------------------------------------------------------------------------
// Changelog: https://github.com/openenergymonitor/emonpi/blob/master/firmware/readme.md
const int firmware_version = 290;                                     //firmware version x 100 e.g 100 = V1.00

//----------------------------emonPi Settings---------------------------------------------------------------------------------------------------------------
bool debug =                   true;
const unsigned long BAUD_RATE=    38400;

const byte Vrms_EU=               230;                              // Vrms for apparent power readings (when no AC-AC voltage sample is present)
const byte Vrms_USA=              120;                              // USA apparent power VRMS
const int TIME_BETWEEN_READINGS=  5000;                             // Time between readings (ms)
const unsigned long RF_RESET_PERIOD=        60000;                            // Time (ms) between RF resets (hack to keep RFM60CW alive)


//http://openenergymonitor.org/emon/buildingblocks/calibration

const float Ical1=                90.9;                             // (2000 turns / 22 Ohm burden) = 90.9
const float Ical2=                90.9;
float Vcal_EU=                    256.8;                             // (230V x 13) / (9V x 1.2) = 276.9 - Calibration for EU AC-AC adapter 77DE-06-09
const float Vcal_USA=             130.0;                             // Calibration for US AC-AC adapter 77DA-10-09
bool USA=                      false;
const byte min_pulsewidth=        60;                                // minimum width of interrupt pulse

const float phase_shift=          1.7;
const int no_of_samples=          1480;
const byte no_of_half_wavelengths= 20;
const int timeout=                2000;                               // emonLib timeout
const int ACAC_DETECTION_LEVEL=   3000;

const byte TEMPERATURE_PRECISION=  12;                                 // 9 (93.8ms),10 (187.5ms) ,11 (375ms) or 12 (750ms) bits equal to resplution of 0.5C, 0.25C, 0.125C and 0.0625C
const byte MaxOnewire=             6;                                  // maximum number of DS18B20 one wire sensors
bool RF_STATUS=                 true;                                  // Turn RF on and off
//-------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------


//----------------------------emonPi V3 hard-wired connections---------------------------------------------------------------------------------------------------------------
const byte LEDpin=                     9;              // emonPi LED - on when HIGH
const byte shutdown_switch_pin =       8;              // Push-to-make - Low when pressed
const byte emonpi_GPIO_pin=            5;              // Connected to Pi GPIO 17, used to activate Pi Shutdown when HIGH
//const byte emonpi_OKK_Tx=              6;            // On-off keying transmission Pin - not populated by default
//const byte emonPi_RJ45_8_IO=           A6;           // RJ45 pin 8 - Analog 6 (D19) - Aux I/O
const byte emonPi_int1=                1;              // RJ45 pin 6 - INT1 - PWM - Dig 3 - default pulse count input
const byte emonPi_int1_pin=            3;              // RJ45 pin 6 - INT1 - PWM - Dig 3 - default pulse count input
//const byte emonPi_int0=                2;            // Default RFM INT (Dig2) - Can be jumpered used JP5 to RJ45 pin 7 - PWM - D2
#define ONE_WIRE_BUS                   4               // DS18B20 Data, RJ45 pin 4
//-------------------------------------------------------------------------------------------------------------------------------------------

//Setup DS128B20
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
byte allAddress [MaxOnewire][8];  // 8 bytes per address
byte numSensors;
//-------------------------------------------------------------------------------------------------------------------------------------------

//-----------------------RFM12B / RFM69CW SETTINGS----------------------------------------------------------------------------------------------------
byte RF_freq=RF12_433MHZ;                                        // Frequency of RF69CW module can be RF12_433MHZ, RF12_868MHZ or RF12_915MHZ. You should use the one matching the module you have.
byte nodeID = 5;                                                 // emonpi node ID
int networkGroup = 210;

typedef struct {
int power1;
int power2;
int power1_plus_2;
int Vrms;
int temp[MaxOnewire];
unsigned long pulseCount;
} PayloadTX;                                                    // create JeeLabs RF packet structure - a neat way of packaging data for RF comms
PayloadTX emonPi;
//-------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------


//Global Variables Energy Monitoring
double Vcal, vrms;
bool ACAC, DS18B20_STATUS;
byte CT_count, Vrms;
unsigned long last_sample=0;                                     // Record millis time of last discrete sample
byte flag;                                                       // flag to record shutdown push button press
volatile byte pulseCount = 0;
unsigned long now =0;
unsigned long pulsetime=0;                                      // Record time of interrupt pulse
unsigned long last_rf_rest=0;                                  // Record time of last RF reset

// RF Global Variables
static byte stack[RF12_MAXDATA+4], top, sendLen, dest;           // RF variables
static char cmd;
static word value;                                               // Used to store serial input
long unsigned int start_press=0;                                 // Record time emonPi shutdown push switch is pressed
bool quiet_mode = true;

const char helpText1[] PROGMEM =                                 // Available Serial Commands
"\n"
"Available commands:\n"
"  <nn> i     - set node IDs (standard node ids are 1..30)\n"
"  <n> b      - set MHz band (4 = 433, 8 = 868, 9 = 915)\n"
"  <nnn> g    - set network group (RFM12 only allows 212, 0 = any)\n"
"  <n> c      - set collect mode (advanced, normally 0)\n"
"  ...,<nn> a - send data packet to node <nn>, request ack\n"
"  ...,<nn> s - send data packet to node <nn>, no ack\n"
"  ...,<n> p  - Set AC Adapter Vcal 1p = UK, 2p = USA\n"
"  v          - Show firmware version\n"
"  <n> q      - set quiet mode (1 = don't report bad packets)\n"
;

//-------------------------------------------------------------------------------------------------------------------------------------------
// SETUP ********************************************************************************************
//-------------------------------------------------------------------------------------------------------------------------------------------
void setup()
{

  delay(100);

  if (USA)
  {
    Vcal = Vcal_USA;                                                       // Assume USA AC/AC adatper is being used, set calibration accordingly
    Vrms = Vrms_USA;
  }
  else
  {
    Vcal = Vcal_EU;
    Vrms = Vrms_EU;
  }

  emonPi_startup();                                                     // emonPi startup proceadure, check for AC waveform and print out debug
  if (RF_STATUS==1) RF_Setup();
  check_for_DS18B20();                               // check for presence of DS18B20 and return number of sensors

  // Detect and startup I2C LCD
  current_lcd_i2c_addr = i2c_lcd_detect(i2c_lcd_address);
  LiquidCrystal_I2C lcd(current_lcd_i2c_addr,16,2);                                   // LCD I2C address to 0x27, 16x2 line display
  emonPi_LCD_Startup(current_lcd_i2c_addr);

  delay(2000);
  CT_Detect();
  serial_print_startup(current_lcd_i2c_addr);

  attachInterrupt(emonPi_int1, onPulse, FALLING);  // Attach pulse counting interrupt on RJ45 (Dig 3 / INT 1)
  emonPi.pulseCount = 0;                                                  // Reset Pulse Count



  ct1.current(1, Ical1);                                     // CT ADC channel 1, calibration.  calibration (2000 turns / 22 Ohm burden resistor = 90.909)
  ct2.current(2, Ical2);                                     // CT ADC channel 2, calibration.

  if (ACAC)                                                           //If AC wavefrom has been detected
  {
    ct1.voltage(0, Vcal, phase_shift);                       // ADC pin, Calibration, phase_shift
    ct2.voltage(0, Vcal, phase_shift);                       // ADC pin, Calibration, phase_shift
  }

} //end setup


//-------------------------------------------------------------------------------------------------------------------------------------------
// LOOP ********************************************************************************************
//-------------------------------------------------------------------------------------------------------------------------------------------
void loop()
{
  now = millis();

  if (USA)
  {
    Vcal = Vcal_USA;                                                       // Assume USA AC/AC adatper is being used, set calibration accordingly
    Vrms = Vrms_USA;
  }
  else
  {
    Vcal = Vcal_EU;
    Vrms = Vrms_EU;
  }

  // Update Vcal
  ct1.voltage(0, Vcal, phase_shift);                       // ADC pin, Calibration, phase_shift
  ct2.voltage(0, Vcal, phase_shift);                       // ADC pin, Calibration, phase_shift

  if (digitalRead(shutdown_switch_pin) == 0 )
    digitalWrite(emonpi_GPIO_pin, HIGH);                                          // if emonPi shutdown butten pressed then send signal to the Pi on GPIO 11
  else
    digitalWrite(emonpi_GPIO_pin, LOW);

  if (Serial.available()){
    handleInput(Serial.read());                                                   // If serial input is received
    double_LED_flash();
  }


  if (RF_STATUS==1){                                                              // IF RF module is present and enabled then perform RF tasks
    if (RF_Rx_Handle()==1) {                                                      // Returns true if RF packet is received
       double_LED_flash();
    }

    send_RF();                                                                    // Transmitt data packets if needed

    if ((now - last_rf_rest) > RF_RESET_PERIOD) {
      rf12_initialize(nodeID, RF_freq, networkGroup);                             // Periodically reset RFM69CW to keep it alive :-(
    }

   }


  if ((now - last_sample) > TIME_BETWEEN_READINGS)
  {
    single_LED_flash();                                                           // single flash of LED on local CT sample

// CT1 --------------------------------------------------------------------------------------------------------------
  if (analogRead(1) > 0){                                                         // If CT is plugged in then sample
    if (ACAC)
    {
      ct1.calcVI(no_of_half_wavelengths,timeout);
      emonPi.power1=ct1.realPower;
      emonPi.Vrms=ct1.Vrms*100;
    }
    else emonPi.power1 = ct1.calcIrms(no_of_samples)*Vrms;                       // Calculate Apparent Power if no AC-AC
  }
  else emonPi.power1=0;                                                         //CT is unplugged

// CT2 --------------------------------------------------------------------------------------------------------------
  if (analogRead(2) > 0){                                                         // If CT is plugged in then sample
   if (ACAC)                                                                      // Read from CT 2
   {
     ct2.calcVI(no_of_half_wavelengths,timeout);
     emonPi.power2=ct2.realPower;
     emonPi.Vrms=ct2.Vrms*100;
   }
   else emonPi.power2 = ct2.calcIrms(no_of_samples)*Vrms;
  }
  else emonPi.power2=0;

  emonPi.power1_plus_2=emonPi.power1 + emonPi.power2;                                       //Calculate power 1 plus power 2 variable for US and solar PV installs

  if ((ACAC==0) && (CT_count > 0)) emonPi.Vrms=Vrms*100;                                   // If no AC wave detected set VRMS constant

  if ((ACAC==1) && (CT_count==0)) {                                                        // If only AC-AC is connected then return just VRMS calculation
     ct1.calcVI(no_of_half_wavelengths,timeout);
     emonPi.Vrms=ct1.Vrms*100;
  }

  //Serial.print(emonPi.pulseCount); Serial.print(" ");delay(5);
   // if (debug==1) {Serial.print(emonPi.power2); Serial.print(" ");delay(5);}


    if (DS18B20_STATUS==1)
    {
      sensors.requestTemperatures();                                        // Send the command to get temperatures
      for(byte j=0;j<numSensors;j++) emonPi.temp[j]=get_temperature(j);
    }

    if (pulseCount)                                                       // if the ISR has counted some pulses, update the total count
    {
      cli();                                                              // Disable interrupt just in case pulse comes in while we are updating the count
      emonPi.pulseCount += pulseCount;
      pulseCount = 0;
      sei();                                                              // Re-enable interrupts
    }
    /*
    Serial.print(CT1); Serial.print(" "); Serial.print(CT2); Serial.print(" "); Serial.print(ACAC); Serial.print(" "); Serial.println  (CT_count);
    Serial.print(emonPi.power1); Serial.print(" ");
    Serial.print(emonPi.power2); Serial.print(" ");
    Serial.print(emonPi.Vrms); Serial.print(" ");
    Serial.println(emonPi.temp[1]);
    */

    send_emonpi_serial();                                             //Send emonPi data to Pi serial using struct packet structure

    last_sample = now;                                           //Record time of sample

    } // end sample


} // end loop---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void single_LED_flash()
{
  digitalWrite(LEDpin, HIGH);  delay(50); digitalWrite(LEDpin, LOW);
}

void double_LED_flash()
{
  digitalWrite(LEDpin, HIGH);  delay(25); digitalWrite(LEDpin, LOW);
  digitalWrite(LEDpin, HIGH);  delay(25); digitalWrite(LEDpin, LOW);
}
