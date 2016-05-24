/*
  
  emonPi  Discrete Sampling 
  
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

#include "EmonLib.h"                                                  // Include EmonLib energy monitoring library https://github.com/openenergymonitor/EmonLib
EnergyMonitor ct1, ct2, ct3, ct4;      

#include <OneWire.h>                                                  // http://www.pjrc.com/teensy/td_libs_OneWire.html
#include <DallasTemperature.h>                                        // http://download.milesburton.com/Arduino/MaximTemperature/DallasTemperature_LATEST.zip

#include <Wire.h>                                                     // Arduino I2C library
//#include <LiquidCrystal_I2C.h>                                        // https://github.com/openenergymonitor/LiquidCrystal_I2C1602V1
//LiquidCrystal_I2C lcd(0x27,16,2);                                     // LCD I2C address to 0x27, 16x2 line display
//const byte firmware_version = 17; 
const byte firmware_version = 21;                                    //firmware version x 10 e.g 10 = V1.0 / 1 = V0.1

//----------------------------emonPi Settings---------------------------------------------------------------------------------------------------------------
boolean debug =                   TRUE; 
const unsigned long BAUD_RATE=    38400;
//const unsigned long BAUD_RATE=    9600;

const byte Vrms_EU=               230;                               // Vrms for apparent power readings (when no AC-AC voltage sample is present)
const byte Vrms_USA=              110;                               // USA apparent power VRMS  
const int TIME_BETWEEN_READINGS=  5000;                             // Time between readings (mS)  


//http://openenergymonitor.org/emon/buildingblocks/calibration

const float Ical1=                90.9;                             // (2000 turns / 22 Ohm burden) = 90.9
const float Ical2=                90.9;                                 
const float Ical3=                90.9;                                 // (2000 turns / 22 Ohm burden) = 90.9
const float Ical4=                16.67;                               // (2000 turns / 120 Ohm burden) = 16.67

float Vcal_EU=                    265.42;                             // (230V x 13) / (9V x 1.2) = 276.9 Calibration for UK AC-AC adapter 77DB-06-09 
//const float Vcal=               260;                                // Calibration for EU AC-AC adapter 77DE-06-09 
const float Vcal_USA=             130.0;                              // Calibration for US AC-AC adapter 77DA-10-09
boolean USA=                      FALSE; 
const byte min_pulsewidth= 110;                              // minimum width of interrupt pulse (default pulse output meters = 100ms)

const float phase_shift=          1.7;
const int no_of_samples=          1480; 
const byte no_of_half_wavelengths= 20;
const int timeout=                2000;                               // emonLib timeout 
const int ACAC_DETECTION_LEVEL=   3000;
#define ASYNC_DELAY 375                                          // DS18B20 conversion delay - 9bit requres 95ms, 10bit 187ms, 11bit 375ms and 12bit resolution takes 750ms

const byte TEMPERATURE_PRECISION=  12;                                 // 9 (93.8ms),10 (187.5ms) ,11 (375ms) or 12 (750ms) bits equal to resplution of 0.5C, 0.25C, 0.125C and 0.0625C
const byte MaxOnewire=             6;                                  // maximum number of DS18B20 one wire sensors           
boolean RF_STATUS=                 1;                                  // Turn RF on and off
//-------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------


//----------------------------emonPi V3 hard-wired connections--------------------------------------------------------------------------------------------------------------- 
//const byte LEDpin=                     9;              // emonPi LED - on when HIGH  (for emonpi)
const byte LEDpin=                     6;    //(from emontx)
//const byte DS18B20_PWR=            19;                             // DS18B20 Power  (from emontx)
const byte DIP_switch1=            8;                              // Voltage selection 230 / 110 V AC (default switch off 230V)  - switch off D8 is HIGH from internal pullup (from emontx)
//const byte shutdown_switch_pin =       8;              // Push-to-make - Low when pressed   (no need of this, we shutdown the system using bbb GPIO)
const byte DIP_switch2=            9;                              // RF node ID (default no chance in node ID, switch on for nodeID -1) switch off D9 is HIGH from internal pullup (from emontx)

const byte emonpi_GPIO_pin=            4;              // Connected to Pi GPIO 17, used to activate Pi Shutdown when HIGH (we don`t need this)
//const byte emonpi_OKK_Tx=              6;            // On-off keying transmission Pin - not populated by default 
//const byte emonPi_RJ45_8_IO=           A3;           // RJ45 pin 8 - Analog 6 (D19) - Aux I/O
const byte battery_voltage_pin=    7;                              // Battery Voltage sample from 3 x AA (from emontx codes)
const byte emonPi_int1=                1;              // RJ45 pin 6 - INT1 - PWM - Dig 3 - default pulse count input
const byte emonPi_int1_pin=            3;              // RJ45 pin 6 - INT1 - PWM - Dig 3 - default pulse count input
//const byte emonPi_int0=                2;            // Default RFM INT (Dig2) - Can be jumpered used JP5 to RJ45 pin 7 - PWM - D2
#define ONE_WIRE_BUS                   5               // DS18B20 Data, RJ45 pin 5 (exchange with emonpi_GPIO_pin based on emontx codes)
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
//uint32_t networkGroup = 3421749817;
uint32_t networkGroup = 210;
   // String api = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";   //API key uint32_t
    //int str_len = api.length() + 1;     // Length (with one extra character for the null terminator)
   // int networkGroup = 0; 


typedef struct { 
int power1;
int power2;
int power3;
int power4;
//int power1_plus_2;
//**************************************for DC voltage reading************* 
int v_battery_bank;  
//*************************************************************************                                                 
int Vrms; 
int temp[MaxOnewire]; 
unsigned long pulseCount;  
} PayloadTX;                                                    // create JeeLabs RF packet structure - a neat way of packaging data for RF comms
PayloadTX emonPi; 
//PayloadTX emontx;

typedef struct { 
  int relay1;
  int relay2;
  int relay3;
  int relay4; 
} Playload;     // create structure - a neat way of packaging data for RF comms
Playload load_monitor ;

static int intRFtime = 600000;                                  // Call rf12_initialize every 10min to restore RF http://openenergymonitor.org/emon/node/5549
//-------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------


//Global Variables Energy Monitoring 
double Vcal, vrms;
boolean CT1, CT2,CT3, CT4, ACAC, DS18B20_STATUS;
//byte CT_count, Vrms;                                             
byte CT_count=0;
byte Vrms; 
unsigned long last_sample=0;                                     // Record millis time of last discrete sample
byte flag;                                                                         // flag to record shutdown push button press
volatile byte pulseCount = 0;
unsigned long now =0;
unsigned long pulsetime=0;                                      // Record time of interrupt pulse          
unsigned long last = 0;
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

//*********************battery voltage reading**********************************************************************
const byte v_battery_pin =    A5;

//int v_battery_pin = A5;  //vout of voltage divider is connected to A5
float vout = 0.0;      //output of voltage regulator
float vin = 0;        //input voltage
float R1 = 9990.0;     //Resistor in ohm for Max V=33
float R2 = 1925.0;      //Resistor in ohm for Max V=33

//float R1 = 10000.0;     //Resistor in ohm for Max V=52     
//float R2 = 1063.0;      //Resistor in ohm for Max V=52
int battery_value = 0;   //analog read 
float previous_reading=0;
float voltage=0;
float  vin_new=0;
float  vin_old=0;
float  vout_old=0;

unsigned long previousMillis = 0;        // will store last time it checked the value from A5         
const long interval = 2000; 

//*****************************************************************************************************************
//-------------------------------------------------------------------------------------------------------------------------------------------
// SETUP ********************************************************************************************
//-------------------------------------------------------------------------------------------------------------------------------------------
void setup()
{ 
  //**************************battery voltage reading***************************
  // pinMode(v_battery_pin, INPUT);
  //------------------------------------from emontx-------------------------------------------------------------
   pinMode(LEDpin, OUTPUT); 
  //pinMode(DS18B20_PWR, OUTPUT); 

  pinMode(emonPi_int1_pin, INPUT_PULLUP);                     // Set emonTx V3.4 interrupt pulse counting pin as input (Dig 3 / INT1)
//  emontx.pulseCount=0;                                        // Make sure pulse count starts at zero

  digitalWrite(LEDpin,HIGH); 

 Serial.begin(9600);
 
  //Serial.print("emonTx V3.4 Discrete Sampling V"); Serial.print(firmware_version*0.1);
  #if (RF69_COMPAT)
    Serial.println(" RFM69CW found ");
  #else
    Serial.println(" RFM12B");
  #endif
  rf12_initialize(nodeID, RF_freq, networkGroup);                         // initialize RFM12B/rfm69CW
  Serial.println("OpenEnergyMonitor.org");
  Serial.println("POST.....wait 10s");
  
  //READ DIP SWITCH POSITIONS 
  pinMode(DIP_switch1, INPUT_PULLUP);
  pinMode(DIP_switch2, INPUT_PULLUP);
  if (digitalRead(DIP_switch1)==LOW) nodeID--;                            // IF DIP switch 1 is switched on then subtract 1 from nodeID
  if (digitalRead(DIP_switch2)==LOW) USA=TRUE;                            // IF DIP switch 2 is switched on then activate USA mode
 //--------------------------------------------------------------------------------------------------------------------------------------------
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
  //emonPi_LCD_Startup(); 
  delay(1500);  
  CT_Detect();
  serial_print_startup();

  attachInterrupt(emonPi_int1, onPulse, FALLING);  // Attach pulse counting interrupt on RJ45 (Dig 3 / INT 1) 
  emonPi.pulseCount = 0;                                                  // Reset Pulse Count 
  for(byte j=0;j<MaxOnewire;j++) 
 //     emontx.temp[j] = 3000;                             // If no temp sensors connected default to status code 3000 
                                                         // will appear as 300 once multipled by 0.1 in emonhub
   
  ct1.current(1, Ical1);                                     // CT ADC channel 1, calibration.  calibration (2000 turns / 22 Ohm burden resistor = 90.909)
  ct2.current(2, Ical2);                                     // CT ADC channel 2, calibration.
  ct3.current(3, Ical3);             // CT ADC channel 3, calibration. 
  ct4.current(4, Ical4);             // CT ADC channel 4, calibration.  calibration (2000 turns / 120 Ohm burden resistor = 16.66) high accuracy @ low power -  4.5kW Max @ 240V 

  if (ACAC)                                                           //If AC wavefrom has been detected 
  {
    ct1.voltage(0, Vcal, phase_shift);                       // ADC pin, Calibration, phase_shift
    ct2.voltage(0, Vcal, phase_shift);                       // ADC pin, Calibration, phase_shift
    ct3.voltage(0, Vcal, phase_shift);          // ADC pin, Calibration, phase_shift
    ct4.voltage(0, Vcal, phase_shift);          // ADC pin, Calibration, phase_shift
    
  }
 
} //end setup


//-------------------------------------------------------------------------------------------------------------------------------------------
// LOOP ********************************************************************************************
//-------------------------------------------------------------------------------------------------------------------------------------------
void loop()
{

      
  
//***********************************************************************************************
//**********************************************************************
   
   // unsigned long start = millis();
  now = millis();
 
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
  
  // Update Vcal
  ct1.voltage(0, Vcal, phase_shift);                       // ADC pin, Calibration, phase_shift
  ct2.voltage(0, Vcal, phase_shift);                       // ADC pin, Calibration, phase_shift
  ct3.voltage(0, Vcal, phase_shift);                       // ADC pin, Calibration, phase_shift
  ct4.voltage(0, Vcal, phase_shift);                       // ADC pin, Calibration, phase_shift

/*
  if (digitalRead(shutdown_switch_pin) == 0 ) 
    digitalWrite(emonpi_GPIO_pin, HIGH);                                          // if emonPi shutdown butten pressed then send signal to the Pi on GPIO 11
  else 
    digitalWrite(emonpi_GPIO_pin, LOW);
  */
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

 
  if ((now - last_sample) > TIME_BETWEEN_READINGS)
  {
    single_LED_flash();                                                            // single flash of LED on local CT sample
    
    if (ACAC)                                                                      // Read from CT 1
    {
     // delay(200);                         //if powering from AC-AC allow time for power supply to settle    
     // emontx.Vrms=0;                      //Set Vrms to zero, this will be overwirtten by CT 1-4
      ct1.calcVI(no_of_half_wavelengths,timeout); emonPi.power1=ct1.realPower;
      emonPi.Vrms=ct1.Vrms*100;
    
    }
    else 
    {
      emonPi.power1 = ct1.calcIrms(no_of_samples)*Vrms;                               // Calculate Apparent Power 1  1480 is  number of samples
      //emonPi.power1=120;
      emonPi.Vrms=Vrms*100;
    }  


  
   if (ACAC)                                                                       // Read from CT 2
   {
    // delay(200);                         //if powering from AC-AC allow time for power supply to settle    
    // emontx.Vrms=0;                      //Set Vrms to zero, this will be overwirtten by CT 1-4
     ct2.calcVI(no_of_half_wavelengths,timeout); emonPi.power2=ct2.realPower;
     emonPi.Vrms=ct2.Vrms*100;
   }
   else 
   {
     emonPi.power2 = ct2.calcIrms(no_of_samples)*Vrms;                               // Calculate Apparent Power 1  1480 is  number of samples
     emonPi.Vrms=Vrms*100;
   }

      if (ACAC)                                                                       // Read from CT 3
   {
     //delay(200);                         //if powering from AC-AC allow time for power supply to settle    
    // emontx.Vrms=0;                      //Set Vrms to zero, this will be overwirtten by CT 1-4
     ct3.calcVI(no_of_half_wavelengths,timeout); emonPi.power3=ct3.realPower;
     emonPi.Vrms=ct3.Vrms*100;
   }
   else 
   {
     emonPi.power3 = ct3.calcIrms(no_of_samples)*Vrms;                               // Calculate Apparent Power 1  1480 is  number of samples
     emonPi.Vrms=Vrms*100;
   }

      if (ACAC)                                                                       // Read from CT 4
   {
     //delay(200);                         //if powering from AC-AC allow time for power supply to settle    
     //emontx.Vrms=0;                      //Set Vrms to zero, this will be overwirtten by CT 1-4
     ct4.calcVI(no_of_half_wavelengths,timeout); emonPi.power4=ct4.realPower;
     emonPi.Vrms=ct4.Vrms*100;
   }
   else 
   {
     emonPi.power4 = ct4.calcIrms(no_of_samples)*Vrms;                               // Calculate Apparent Power 1  1480 is  number of samples
     emonPi.Vrms=Vrms*100;
   }

  // emonPi.power1_plus_2=emonPi.power1 + emonPi.power2;                            //Create power 1 plus power 2 variable for US and solar PV installs
// emonPi.power1_plus_2=11; 
 //***********************for battery voltage reading********************************************
   battery_value = analogRead(v_battery_pin);
   delay(100);
   vout = (battery_value * 5) / 1024.0; 
   vin_new=vout/(R2/(R1+R2));

  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis >= interval) 
  {
    previousMillis = currentMillis;     
    previous_reading = analogRead(v_battery_pin);
    vout_old = (previous_reading * 5) / 1024.0; 
    vin_old=vout_old / (R2/(R1+R2));
      vin=(vin_new+vin_old)/2;   // final data to send in emonhub 
      //Serial.println(vin); 
  } 
   /*
       Serial.println("it**************");
   Serial.print("old reading :"); 
    Serial.println(vin_old); 
     Serial.print("New reading= ");
    Serial.println(vin_new); 
    Serial.print("vin:");
    Serial.println(vin);
    */
    
   // emonPi.v_battery_bank=vin*100;
  // emonPi.v_battery_bank=vin*100;
//    emontx.Vrms= battery_voltage;
emonPi.v_battery_bank=vin*100;



   
  //Serial.print(emonPi.pulseCount); Serial.print(" ");delay(5);
   // if (debug==1) {Serial.print(emonPi.power2); Serial.print(" ");delay(5);}  
    if (!ACAC){                                                                         // read battery voltage if powered by DC
    int battery_voltage=analogRead(battery_voltage_pin) * 0.681322727;                // 6.6V battery = 3.3V input = 1024 ADC

  }

    if (DS18B20_STATUS==1) 
    {
     // digitalWrite(DS18B20_PWR, HIGH); 
      Sleepy::loseSomeTime(50);
      sensors.requestTemperatures();                                        // Send the command to get temperatures
      for(byte j=0;j<numSensors;j++) emonPi.temp[j]=get_temperature(j); 
      //sensors.setResolution(allAddress[j], TEMPERATURE_PRECISION);                    // and set the a to d conversion resolution of each.
      //       emontx.temp[j]=get_temperature(j); 
     // digitalWrite(DS18B20_PWR, LOW);
    }                                                                           
    
    if (pulseCount)                                                       // if the ISR has counted some pulses, update the total count
    {
      cli();                                                              // Disable interrupt just in case pulse comes in while we are updating the count
      emonPi.pulseCount += pulseCount;

      pulseCount = 0;
      sei();                                                              // Re-enable interrupts
    } 

    /*    //----------------------------------------------------------------------------------------
  if (debug==1) {
    Serial.print(emonPi.power1); Serial.print(" ");
    Serial.print(emonPi.power2); Serial.print(" ");
    Serial.print(emonPi.power3); Serial.print(" ");
    Serial.print(emonPi.power4); Serial.print(" ");
    Serial.print(emonPi.Vrms); Serial.print(" ");
    Serial.print(emonPi.pulseCount); Serial.print(" ");
    if (DS18B20_STATUS==1){
      for(byte j=0;j<numSensors;j++){
        Serial.print(emonPi.temp[j]);
       Serial.print(" ");
      } 
    }
    Serial.print(" ");
    Serial.print(millis()-last);
    last = millis();
    Serial.println(" ");
    delay(50);
  }
  if (ACAC) {digitalWrite(LEDpin, HIGH); delay(200); digitalWrite(LEDpin, LOW);}    // flash LED if powered by AC

//  unsigned long runtime = millis() - start;
//  unsigned long sleeptime = (TIME_BETWEEN_READINGS*1000) - runtime - 100;
  
  if (ACAC) {                                                               // If powered by AC-AC adaper (mains power) then delay instead of sleep
//    delay(sleeptime);
  } else {                                                                  // if powered by battery then sleep rather than dealy and disable LED to lower energy consumption  
                                   // lose an additional 500ms here (measured timing)
//    Sleepy::loseSomeTime(sleeptime-500);                                    // sleep or delay in seconds 
  }
        //-------------------------------------------------------------------------------------
*/
    
    /*Serial.print(emonPi.power1); Serial.print(" ");
    Serial.print(emonPi.power2); Serial.print(" ");
    Serial.print(emonPi.Vrms); Serial.print(" ");
    Serial.println(emonPi.temp);
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




