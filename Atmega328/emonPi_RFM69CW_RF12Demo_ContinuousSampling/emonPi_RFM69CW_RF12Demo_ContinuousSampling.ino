/*
  
  emonPi  Continuous Sampling 
  
  If AC-AC adapter is detected assume emonPi is also powered from adapter (jumper shorted) and take Real Power Readings and disable sleep mode to keep load on power supply constant
  If AC-AC addapter is not detected assume powering from battereis / USB 5V AC sample is not present so take Apparent Power Readings and enable sleep mode
  
  Transmitt values via RFM69CW radio
  
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


*/

#define emonTxV3                                                      // Tell emonLib this is the emonPi V3 - don't read Vcc assume Vcc = 3.3V as is always the case on emonPi eliminates bandgap error and need for calibration http://harizanov.com/2013/09/thoughts-on-avr-adc-accuracy/
#define RF69_COMPAT 1                                                 // Set to 1 if using RFM69CW or 0 is using RFM12B

#include <JeeLib.h>                                                   // https://github.com/jcw/jeelib - Tested with JeeLib 3/11/14
#include <avr/pgmspace.h>
#include <util/parity.h>
ISR(WDT_vect) { Sleepy::watchdogEvent(); }                            // Attached JeeLib sleep function to Atmega328 watchdog -enables MCU to be put into sleep mode inbetween readings to reduce power consumption 

#include "EmonLibCM.h"

#include <OneWire.h>                                                  // http://www.pjrc.com/teensy/td_libs_OneWire.html
#include <DallasTemperature.h>                                        // http://download.milesburton.com/Arduino/MaximTemperature/DallasTemperature_LATEST.zip

#include <Wire.h>                                                     // Arduino I2C library
#include <LiquidCrystal_I2C.h>                                        // https://github.com/openenergymonitor/LiquidCrystal_I2C1602V1
LiquidCrystal_I2C lcd(0x27,16,2);                                     // LCD I2C address to 0x27, 16x2 line display

const byte firmware_version = 20;                                    //firmware version x 10 e.g 10 = V1.0 / 1 = V0.1

//----------------------------emonPi Settings---------------------------------------------------------------------------------------------------------------
boolean debug =                   TRUE; 
const unsigned long BAUD_RATE=    38400;

const byte Vrms_EU=               230;                               // Vrms for apparent power readings (when no AC-AC voltage sample is present)
const byte Vrms_USA=              110;                               // USA apparent power VRMS  
const int TIME_BETWEEN_READINGS=  5;                             // Time between readings (mS)  


//http://openenergymonitor.org/emon/buildingblocks/calibration

const float Ical1=                90.9;                             // (2000 turns / 22 Ohm burden) = 90.9
const float Ical2=                90.9;                                 
float Vcal_EU=                    260.4;                             // (230V x 13) / (9V x 1.2) = 276.9 Calibration for UK AC-AC adapter 77DB-06-09 
//const float Vcal=               260;                                // Calibration for EU AC-AC adapter 77DE-06-09 
const float Vcal_USA=             130.0;                              // Calibration for US AC-AC adapter 77DA-10-09
boolean USA=                      FALSE; 
const byte min_pulsewidth= 110;                              // minimum width of interrupt pulse (default pulse output meters = 100ms)

const byte TEMPERATURE_PRECISION=  12;                                 // 9 (93.8ms),10 (187.5ms) ,11 (375ms) or 12 (750ms) bits equal to resplution of 0.5C, 0.25C, 0.125C and 0.0625C
const byte MaxOnewire=             6;                                  // maximum number of DS18B20 one wire sensors           
boolean RF_STATUS=                 1;                                  // Turn RF on and off
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
byte RF_freq = RF12_433MHZ;                                        // Frequency of RF69CW module can be RF12_433MHZ, RF12_868MHZ or RF12_915MHZ. You should use the one matching the module you have.
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
boolean CT1, CT2, ACAC, DS18B20_STATUS;
byte CT_count, Vrms;                                             
unsigned long last_sample=0;                                     // Record millis time of last discrete sample
byte flag;                                                                         // flag to record shutdown push button press
volatile byte pulseCount = 0;
unsigned long now =0;
unsigned long pulsetime=0;                                      // Record time of interrupt pulse          

// RF Global Variables 
static byte stack[RF12_MAXDATA+4], top, sendLen, dest;           // RF variables 
static char cmd;
static word value;                                               // Used to store serial input
long unsigned int start_press=0;                                 // Record time emonPi shutdown push switch is pressed

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
;

//-------------------------------------------------------------------------------------------------------------------------------------------
// SETUP ********************************************************************************************
//-------------------------------------------------------------------------------------------------------------------------------------------
void setup()
{ 
  delay(100);
  if (USA==TRUE) 
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
  byte numSensors =  check_for_DS18B20();                               // check for presence of DS18B20 and return number of sensors 
  emonPi_LCD_Startup(); 
  delay(1500);  
  CT_Detect();
  serial_print_startup();
   
  attachInterrupt(emonPi_int1, onPulse, FALLING);  // Attach pulse counting interrupt on RJ45 (Dig 3 / INT 1) 
  emonPi.pulseCount = 0;                                                  // Reset Pulse Count 
   
  EmonLibCM_number_of_channels(2);                          // number of current channels
  EmonLibCM_cycles_per_second(50);                          // frequency 50Hz, 60Hz
  EmonLibCM_datalog_period(TIME_BETWEEN_READINGS);          // period of readings in seconds

  EmonLibCM_min_startup_cycles(10);      // number of cycles to let ADC run before starting first actual measurement
                                         // larger value improves stability if operated in stop->sleep->start mode

  EmonLibCM_voltageCal(Vcal*(3.3/1023));            // 260.4 * (3.3/1023)
  
  EmonLibCM_currentCal(0,Ical1*(3.3/1023));  // 2000 turns / 22 Ohms burden resistor
  EmonLibCM_currentCal(1,Ical2*(3.3/1023));  // 2000 turns / 22 Ohms burden resistor

  EmonLibCM_phaseCal(0,0.22);
  EmonLibCM_phaseCal(1,0.41);
  
  EmonLibCM_Init();
} //end setup


//-------------------------------------------------------------------------------------------------------------------------------------------
// LOOP ********************************************************************************************
//-------------------------------------------------------------------------------------------------------------------------------------------
void loop()
{
  now = millis();

  if (digitalRead(shutdown_switch_pin) == 0 ) 
    digitalWrite(emonpi_GPIO_pin, HIGH);                                          // if emonPi shutdown butten pressed then send signal to the Pi on GPIO 11
  else 
    digitalWrite(emonpi_GPIO_pin, LOW);
  
  if (Serial.available()){
    handleInput(Serial.read());                                                   // If serial input is received
    double_LED_flash();
  }                                             
      

  if (RF_STATUS==1){                                                              // IF RF module is present and enabled then perform RF tasks
    if (RF_Rx_Handle()==1)
    {                                                                             // Returns true if RF packet is received                                            
       double_LED_flash(); 
    }
    send_RF();                                                                    // Transmitt data packets if needed 
  }

  if (EmonLibCM_Ready())   
  {
    if (EmonLibCM_ACAC) {
      emonPi.Vrms = EmonLibCM_Vrms*100;
      emonPi.power1 = EmonLibCM_getRealPower(0);
      emonPi.power2 = EmonLibCM_getRealPower(1);
      // emonPi.wh1 = EmonLibCM_getWattHour(0);
      // emonPi.wh2 = EmonLibCM_getWattHour(1);
    } else {
      emonPi.Vrms = Vrms*100;
      emonPi.power1 = Vrms * EmonLibCM_getIrms(0);
      emonPi.power2 = Vrms * EmonLibCM_getIrms(1);
      // emonPi.wh1 = 0;
      // emonPi.wh2 = 0;
    }
  }
  
  if ((now - last_sample) > TIME_BETWEEN_READINGS*1000)
  {
    single_LED_flash();                                                            // single flash of LED on local CT sample
    emonPi.power1_plus_2=emonPi.power1 + emonPi.power2;                            // Create power 1 plus power 2 variable for US and solar PV installs

    // Serial.print(emonPi.pulseCount); Serial.print(" ");delay(5);
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
    
    /*Serial.print(emonPi.power1); Serial.print(" ");
    Serial.print(emonPi.power2); Serial.print(" ");
    Serial.print(emonPi.Vrms); Serial.print(" ");
    Serial.println(emonPi.temp);
    */
    send_emonpi_serial();                                             // Send emonPi data to Pi serial using struct packet structure
    last_sample = now;                                                // Record time of sample  
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




