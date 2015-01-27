/*
  
  emonPi  Discrete Sampling 
  
  If AC-AC adapter is detected assume emonPi is also powered from adapter (jumper shorted) and take Real Power Readings and disable sleep mode to keep load on power supply constant
  If AC-AC addapter is not detected assume powering from battereis / USB 5V AC sample is not present so take Apparent Power Readings and enable sleep mode
  
  Transmitt values via RFM69CW radio
  
   ----------------------------------------b-
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


*/

#define emonTxV3                                                      // Tell emonLib this is the emonPi V3 - don't read Vcc assume Vcc = 3.3V as is always the case on emonPi eliminates bandgap error and need for calibration http://harizanov.com/2013/09/thoughts-on-avr-adc-accuracy/
#define RF69_COMPAT 1                                                 // Set to 1 if using RFM69CW or 0 is using RFM12B

#include <JeeLib.h>                                                   // https://github.com/jcw/jeelib - Tested with JeeLib 3/11/14
#include <avr/pgmspace.h>
#include <util/parity.h>
ISR(WDT_vect) { Sleepy::watchdogEvent(); }                            // Attached JeeLib sleep function to Atmega328 watchdog -enables MCU to be put into sleep mode inbetween readings to reduce power consumption 

#include "EmonLib.h"                                                  // Include EmonLib energy monitoring library https://github.com/openenergymonitor/EmonLib
EnergyMonitor ct1, ct2, ct3, ct4;       

#include <OneWire.h>                                                  // http://www.pjrc.com/teensy/td_libs_OneWire.html
#include <DallasTemperature.h>                                        // http://download.milesburton.com/Arduino/MaximTemperature/DallasTemperature_LATEST.zip

#include <Wire.h>                                                     // Arduino I2C library
#include <LiquidCrystal_I2C.h>                                        // https://github.com/openenergymonitor/LiquidCrystal_I2C1602V1
LiquidCrystal_I2C lcd(0x27,16,2);                                     // LCD I2C address to 0x27, 16x2 line display

float firmware_version = 0.1;

//----------------------------emonPi Settings---------------------------------------------------------------------------------------------------------------
boolean debug =                   TRUE; 
const int BAUD_RATE=              9600;

const byte Vrms=                  230;                               // Vrms for apparent power readings (when no AC-AC voltage sample is present)
const int TIME_BETWEEN_READINGS=  2000;                              // Time between readings (mS)  


//http://openenergymonitor.org/emon/buildingblocks/calibration

const float Ical1=                60.606;                             // emonpi Calibration factor = (100A / 0.05A) / 33 Ohms
const float Ical2=                60.606;                                 
float Vcal=                       268.97;                             // (230V x 13) / (9V x 1.2) = 276.9 Calibration for UK AC-AC adapter 77DB-06-09 
//const float Vcal=               260;                                // Calibration for EU AC-AC adapter 77DE-06-09 
const float Vcal_USA=             130.0;                              // Calibration for US AC-AC adapter 77DA-10-09
boolean USA=                      FALSE; 


const float phase_shift=          1.7;
const int no_of_samples=          1480; 
const int no_of_half_wavelengths= 20;
const int timeout=                2000;                               // emonLib timeout 
const int ACAC_DETECTION_LEVEL=   3000;
const int TEMPERATURE_PRECISION=  11;                                 // 9 (93.8ms),10 (187.5ms) ,11 (375ms) or 12 (750ms) bits equal to resplution of 0.5C, 0.25C, 0.125C and 0.0625C
#define ASYNC_DELAY               375                                 // DS18B20 conversion delay - 9bit requres 95ms, 10bit 187ms, 11bit 375ms and 12bit resolution takes 750ms
boolean RF_STATUS=                1;
//-------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------


//----------------------------emonPi V3 hard-wired connections--------------------------------------------------------------------------------------------------------------- 
const byte LEDpin=                     9;              // emonPi LED - on when HIGH
const byte shutdown_switch_pin =       8;              // Push-to-make - Low when pressed
const byte emonpi_GPIO_pin=            5;              // Connected to Pi GPIO 17, used to activate Pi Shutdown when HIGH
//const byte emonpi_OKK_Tx=              6;              // On-off keying transmission Pin - not populated by default 
//const byte emonPi_RJ45_8_IO=           A6;             // RJ45 pin 8 - Analog 6 (D19) - Aux I/O
//const byte emonPi_int1=                3;              // RJ45 pin 6 - INT1 - PWM - D3 - default pukse count input
//const byte emonPi_int0=                2;              // Default RFM INT - Can be jumpered used JP5 to RJ45 pin 7 - PWM - D2
#define ONE_WIRE_BUS                   4               // DS18B20 Data, RJ45 pin 4
//-------------------------------------------------------------------------------------------------------------------------------------------

//Setup DS128B20
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
int numSensors;  
byte allAddress [4][8];  // 8 bytes per address, MAX 4!
//-------------------------------------------------------------------------------------------------------------------------------------------

//-----------------------RFM12B / RFM69CW SETTINGS----------------------------------------------------------------------------------------------------
byte RF_freq=RF12_433MHZ;                                        // Frequency of RF69CW module can be RF12_433MHZ, RF12_868MHZ or RF12_915MHZ. You should use the one matching the module you have.
byte nodeID = 5;                                                   // emonpi node ID
int networkGroup = 210;  
typedef struct { int power1, power2, Vrms, temp; } PayloadTX;     // create structure - a neat way of packaging data for RF comms
  PayloadTX emonPi; 
//-------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------


//Global Variables 
double vrms;
boolean CT1, CT2, ACAC, DS18B20_STATUS;
byte CT_count=0;                                                 // Number of CT sensors detected
byte flag;                                                       // flag to record shutdown push button press
static byte stack[RF12_MAXDATA+4], top, sendLen, dest;                                  // RF variables 
static char cmd;
static word value;                                               // Used to store serial input
long unsigned int start_press=0;                                 // Record time emonPi shutdown push switch is pressed
unsigned long last_sample=0;                                     // Record millis time of last discrete sample


const char helpText1[] PROGMEM =
"\n"
"Available commands:\n"
"  <nn> i     - set node ID (standard node ids are 1..30)\n"
"  <n> b      - set MHz band (4 = 433, 8 = 868, 9 = 915)\n"
"  <nnn> g    - set network group (RFM12 only allows 212, 0 = any)\n"
"  <n> c      - set collect mode (advanced, normally 0)\n"
"  ...,<nn> a - send data packet to node <nn>, request ack\n"
"  ...,<nn> s - send data packet to node <nn>, no ack\n"
;

//-------------------------------------------------------------------------------------------------------------------------------------------
// SETUP ********************************************************************************************
//-------------------------------------------------------------------------------------------------------------------------------------------
void setup()
{ 
  delay(100);
  
  if (RF_STATUS==1) RF_Setup(); 
  int numsensors =  check_for_DS18B20();                //check for presence of DS18B20 and return number of sensors 
  emonPi_startup();                                                     // emonPi startup proceadure, check for AC waveform and print out debug
  emonPi_LCD_Startup();                                                 // Startup emonPi LCD and print startup notice
  CT_Detect();
  
   
  serial_print_startup();

 

  if (CT1) ct1.current(1, Ical1);                                     // CT ADC channel 1, calibration.  calibration (2000 turns / 22 Ohm burden resistor = 90.909)
  if (CT2) ct2.current(2, Ical2);                                    // CT ADC channel 2, calibration.

  if (ACAC)                                                                    //If AC wavefrom has been detected 
  {
    if (CT1) ct1.voltage(0, Vcal, phase_shift);          // ADC pin, Calibration, phase_shift
    if (CT2) ct2.voltage(0, Vcal, phase_shift);          // ADC pin, Calibration, phase_shift
  }
 
} //end setup


//-------------------------------------------------------------------------------------------------------------------------------------------
// LOOP ********************************************************************************************
//-------------------------------------------------------------------------------------------------------------------------------------------
void loop()
{
  digitalWrite(LEDpin, LOW);

  if (digitalRead(shutdown_switch_pin) == 0 ) shutdown_sequence();    // if emonPi shutdown butten pressed then initiate shutdown sequence
  
  if (Serial.available())                                             // If serial input is received
      handleInput(Serial.read());

  if (RF_STATUS==1){                                                  // IF RF module is present and enabled then perform RF tasks
    if (RF_Rx_Handle()==1){                                           // Returns true if RF packet is received                                            
      digitalWrite(LEDpin, HIGH); 
    }
    //send_RF();                                                        // Transmitt data packets if needed
  }

 
  
  if ((millis() - last_sample) > TIME_BETWEEN_READINGS)
  {
    if (ACAC) {
      delay(200);                                //if powering from AC-AC allow time for power supply to settle    
      emonPi.Vrms=0;                      //Set Vrms to zero, this will be overwirtten by either CT 1-2
    }
    
    if (CT1) 
    {
     if (ACAC) 
     {
       ct1.calcVI(no_of_half_wavelengths,timeout); emonPi.power1=ct1.realPower;
       emonPi.Vrms=ct1.Vrms*100;
     }
     else
       emonPi.power1 = ct1.calcIrms(no_of_samples)*Vrms;                               // Calculate Apparent Power 1  1480 is  number of samples
     //if (debug==1) {Serial.print(emonPi.power1); Serial.print(" ");delay(5);} 

    }
    
    if (CT2) 
    {
     if (ACAC) 
     {
       ct2.calcVI(no_of_half_wavelengths,timeout); emonPi.power2=ct2.realPower;
       emonPi.Vrms=ct2.Vrms*100;
     }
     else
       emonPi.power2 = ct2.calcIrms(no_of_samples)*Vrms;                               // Calculate Apparent Power 1  1480 is  number of samples
    // if (debug==1) {Serial.print(emonPi.power2); Serial.print(" ");delay(5);}  
    }

    if (DS18B20_STATUS==1) emonPi.temp=get_temperature();                                                                             //Get temperature from DS18B20 if sensor is present
          
    //Serial.print(emonPi.temp);
    
    send_emonpi_serial();                                        //Send emonPi data to Pi serial using struct packet structure 
       
    //delay(TIME_BETWEEN_READINGS*1000);
    
    digitalWrite(LEDpin, HIGH);   
    last_sample = millis();                               //Record time of sample  
    }

  
    
 
} // end loop---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------





