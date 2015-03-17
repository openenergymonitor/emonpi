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
EnergyMonitor ct1, ct2;       

#include <OneWire.h>                                                  // http://www.pjrc.com/teensy/td_libs_OneWire.html
#include <DallasTemperature.h>                                        // http://download.milesburton.com/Arduino/MaximTemperature/DallasTemperature_LATEST.zip

#include <Wire.h>                                                     // Arduino I2C library
#include <LiquidCrystal_I2C.h>                                        // https://github.com/openenergymonitor/LiquidCrystal_I2C1602V1
LiquidCrystal_I2C lcd(0x27,16,2);                                     // LCD I2C address to 0x27, 16x2 line display

float firmware_version = 0.1;

//----------------------------emonPi Settings---------------------------------------------------------------------------------------------------------------
boolean debug =                   TRUE; 
const unsigned long BAUD_RATE=    38400;

const byte Vrms=                  230;                               // Vrms for apparent power readings (when no AC-AC voltage sample is present)
const int TIME_BETWEEN_READINGS=  10000;                             // Time between readings (mS)  


//http://openenergymonitor.org/emon/buildingblocks/calibration

const float Ical1=                90.9;                             // (2000 turns / 22 Ohm burden) = 90.9
const float Ical2=                90.9;                                 
float Vcal_EU=                    268.97;                             // (230V x 13) / (9V x 1.2) = 276.9 Calibration for UK AC-AC adapter 77DB-06-09 
//const float Vcal=               260;                                // Calibration for EU AC-AC adapter 77DE-06-09 
const float Vcal_USA=             130.0;                              // Calibration for US AC-AC adapter 77DA-10-09
boolean USA=                      FALSE; 


const float phase_shift=          1.7;
const int no_of_samples=          1480; 
const byte no_of_half_wavelengths= 20;
const int timeout=                2000;                               // emonLib timeout 
const int ACAC_DETECTION_LEVEL=   3000;

const byte TEMPERATURE_PRECISION=  12;                                 // 9 (93.8ms),10 (187.5ms) ,11 (375ms) or 12 (750ms) bits equal to resplution of 0.5C, 0.25C, 0.125C and 0.0625C
const byte MaxOnewire=             6;                                  // maximum number of DS18B20 one wire sensors           
boolean RF_STATUS=                 1;                                  // Turn RF on and off
//-------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------


//----------------------------emonPi V3 hard-wired connections--------------------------------------------------------------------------------------------------------------- 
const byte LEDpin=                     9;              // emonPi LED - on when HIGH
const byte shutdown_switch_pin =       8;              // Push-to-make - Low when pressed
const byte emonpi_GPIO_pin=            5;              // Connected to Pi GPIO 17, used to activate Pi Shutdown when HIGH
//const byte emonpi_OKK_Tx=              6;              // On-off keying transmission Pin - not populated by default 
//const byte emonPi_RJ45_8_IO=           A6;             // RJ45 pin 8 - Analog 6 (D19) - Aux I/O
const byte emonPi_int1=                3;              // RJ45 pin 6 - INT1 - PWM - Dig 3 - default pulse count input
//const byte emonPi_int0=                2;              // Default RFM INT (Dig2) - Can be jumpered used JP5 to RJ45 pin 7 - PWM - D2
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
byte nodeID = 5;                                                   // emonpi node ID
int networkGroup = 210;  

typedef struct { 
int power1;
int power2;
unsigned long pulseCount; 
int Vrms; 
int temp[MaxOnewire]; 
} PayloadTX;     // create structure - a neat way of packaging data for RF comms
PayloadTX emonPi; 

//-------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------


//Global Variables Energy Monitoring 
double vrms, Vcal;
boolean CT1, CT2, ACAC, DS18B20_STATUS;
byte CT_count=0;                                                 // Number of CT sensors detected
unsigned long last_sample=0;                                     // Record millis time of last discrete sample
byte flag;                                                       // flag to record shutdown push button press


// RF Global Variables 
static byte stack[RF12_MAXDATA+4], top, sendLen, dest;           // RF variables 
static char cmd;
static word value;                                               // Used to store serial input
long unsigned int start_press=0;                                 // Record time emonPi shutdown push switch is pressed

const char helpText1[] PROGMEM =                                 // Available Serial Commands 
"\n"
"Available commands:\n"
"  <nn> i     - set node ID (standard node ids are 1..30)\n"
"  <n> b      - set MHz band (4 = 433, 8 = 868, 9 = 915)\n"
"  <nnn> g    - set network group (RFM12 only allows 212, 0 = any)\n"
"  <n> c      - set collect mode (advanced, normally 0)\n"
"  ...,<nn> a - send data packet to node <nn>, request ack\n"
"  ...,<nn> s - send data packet to node <nn>, no ack\n"
"  ...,<n> v  - Set AC Adapter Vcal 1v = UK, 2v = USA\n"
;

//-------------------------------------------------------------------------------------------------------------------------------------------
// SETUP ********************************************************************************************
//-------------------------------------------------------------------------------------------------------------------------------------------
void setup()
{ 
  delay(100);

  attachInterrupt(1, onPulse, FALLING);                                 // Attach pulse counting interrupt on RJ45 (Dig 3 / INT 1)
  emonPi.pulseCount = 0;                                                // Reset Pulse Count 

   if (USA==TRUE) Vcal=Vcal_USA;                                        // Assume USA AC/AC adatper is being used, set calibration accordingly 
    else Vcal=Vcal_EU;
  
  if (RF_STATUS==1) RF_Setup(); 
  byte numSensors =  check_for_DS18B20();                               // check for presence of DS18B20 and return number of sensors 
  emonPi_startup();                                                     // emonPi startup proceadure, check for AC waveform and print out debug
  emonPi_LCD_Startup();                                                 // Startup emonPi LCD and print startup notice
  CT_Detect();
  
   
  serial_print_startup();

  if (CT1) ct1.current(1, Ical1);                                     // CT ADC channel 1, calibration.  calibration (2000 turns / 22 Ohm burden resistor = 90.909)
  if (CT2) ct2.current(2, Ical2);                                     // CT ADC channel 2, calibration.

  if (ACAC)                                                           //If AC wavefrom has been detected 
  {
    if (CT1) ct1.voltage(0, Vcal, phase_shift);                       // ADC pin, Calibration, phase_shift
    if (CT2) ct2.voltage(0, Vcal, phase_shift);                       // ADC pin, Calibration, phase_shift
  }
 
} //end setup


//-------------------------------------------------------------------------------------------------------------------------------------------
// LOOP ********************************************************************************************
//-------------------------------------------------------------------------------------------------------------------------------------------
void loop()
{
 
    if (USA==TRUE) Vcal=Vcal_USA;                                                     // Assume USA AC/AC adatper is being used, set calibration accordingly 
    else Vcal=Vcal_EU;

  if (digitalRead(shutdown_switch_pin) == 0 ) 
    digitalWrite(emonpi_GPIO_pin, HIGH);     // if emonPi shutdown butten pressed then send signal to the Pi on GPIO 11
  else 
    digitalWrite(emonpi_GPIO_pin, LOW);
  
  if (Serial.available()){
    handleInput(Serial.read()); // If serial input is received
    //digitalWrite(LEDpin, HIGH);  delay(200); digitalWrite(LEDpin, LOW); 
  }                                             
      

  if (RF_STATUS==1){                                                  // IF RF module is present and enabled then perform RF tasks
    if (RF_Rx_Handle()==1){                                           // Returns true if RF packet is received                                            
       digitalWrite(LEDpin, HIGH);  delay(200); digitalWrite(LEDpin, LOW);   
    }
    send_RF();                                                        // Transmitt data packets if needed NEEDS TESTING
  }

 
  
  if ((millis() - last_sample) > TIME_BETWEEN_READINGS)
  {
    //digitalWrite(LEDpin, HIGH);  delay(200); digitalWrite(LEDpin, LOW); 

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

    if (DS18B20_STATUS==1) 
    {
       sensors.requestTemperatures();                                        // Send the command to get temperatures
      for(byte j=0;j<numSensors;j++) emonPi.temp[j]=get_temperature(j); 
    }                                                                           
            
    
    /*Serial.print(emonPi.power1); Serial.print(" ");
    Serial.print(emonPi.power2); Serial.print(" ");
    Serial.print(emonPi.Vrms); Serial.print(" ");
    Serial.println(emonPi.temp);
    */
    send_emonpi_serial();                                        //Send emonPi data to Pi serial using struct packet structure
    
    last_sample = millis();                               //Record time of sample  
    
    } // end sample

  
    
 
} // end loop---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------





