#line 1 "/home/glyn/Dropbox/Energy Monitoring/Open Energy Monitor/emonTx/emonPi/Software/emonpi/Atmega328/emonPi_RFM69CW_RF12Demo_DiscreteSampling/emonPi_RFM69CW_RF12Demo_DiscreteSampling.ino"
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
#include <Arduino.h>
void onPulse();
void RF_Setup();
boolean RF_Rx_Handle();
void send_RF();
static void handleInput(char c);
static byte bandToFreq(byte band);
void emonPi_LCD_Startup();
void serial_print_startup();
void send_emonpi_serial();
static void showString(PGM_P s);
double calc_rms(int pin, int samples);
void emonPi_startup();
void CT_Detect();
byte check_for_DS18B20();
int get_temperature(byte sensor);
#line 44
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

const float Ical1=                60.606;                             // emonpi Calibration factor = (100A / 0.05A) / 33 Ohms
const float Ical2=                60.606;                                 
float Vcal_EU=                    268.97;                             // (230V x 13) / (9V x 1.2) = 276.9 Calibration for UK AC-AC adapter 77DB-06-09 
//const float Vcal=               260;                                // Calibration for EU AC-AC adapter 77DE-06-09 
const float Vcal_USA=             130.0;                              // Calibration for US AC-AC adapter 77DA-10-09
boolean USA=                      FALSE; 


const float phase_shift=          1.7;
const int no_of_samples=          1480; 
const byte no_of_half_wavelengths= 20;
const int timeout=                2000;                               // emonLib timeout 
const int ACAC_DETECTION_LEVEL=   3000;
const int ppwh=                   1;                                  // Number of Wh elapsed per pulse (*1000 per Kwh)

const byte TEMPERATURE_PRECISION=  12;                                 // 9 (93.8ms),10 (187.5ms) ,11 (375ms) or 12 (750ms) bits equal to resplution of 0.5C, 0.25C, 0.125C and 0.0625C
const byte MaxOnewire=             6;                                  // maximum number of DS18B20 one wire sensors           
boolean RF_STATUS=                 1;
//-------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------


//----------------------------emonPi V3 hard-wired connections--------------------------------------------------------------------------------------------------------------- 
const byte LEDpin=                     9;              // emonPi LED - on when HIGH
const byte shutdown_switch_pin =       8;              // Push-to-make - Low when pressed
const byte emonpi_GPIO_pin=            5;              // Connected to Pi GPIO 17, used to activate Pi Shutdown when HIGH
//const byte emonpi_OKK_Tx=              6;              // On-off keying transmission Pin - not populated by default 
//const byte emonPi_RJ45_8_IO=           A6;             // RJ45 pin 8 - Analog 6 (D19) - Aux I/O
const byte emonPi_int1=                1;              // RJ45 pin 6 - INT1 - PWM - Dig 3 - default pulse count input
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
typedef struct { int power1, power2, pulse_elapsedkWh, Vrms, temp[MaxOnewire]; } PayloadTX;     // create structure - a neat way of packaging data for RF comms
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

// Pulse Counting                          
long pulseCount = 0;                                             // Number of pulses, used to measure energy.
unsigned long pulseTime,lastPulseTime;                           // Record time between pulses 
double pulse_elapsedkWh;                                         // Elapsed Kwh from pulse couting

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

  attachInterrupt(emonPi_int1, onPulse, FALLING);                       // Attach pulse counting interrupt on RJ45

   if (USA==TRUE) Vcal=Vcal_USA;                                        // Assume USA AC/AC adatper is being used, set calibration accordingly 
    else Vcal=Vcal_EU;
  
  if (RF_STATUS==1) RF_Setup(); 
  byte numSensors =  check_for_DS18B20();                              // check for presence of DS18B20 and return number of sensors 
  emonPi_startup();                                                   // emonPi startup proceadure, check for AC waveform and print out debug
  emonPi_LCD_Startup();                                               // Startup emonPi LCD and print startup notice
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






#line 1 "/home/glyn/Dropbox/Energy Monitoring/Open Energy Monitor/emonTx/emonPi/Software/emonpi/Atmega328/emonPi_RFM69CW_RF12Demo_DiscreteSampling/emonPi_Interrupt_Pulse.ino"
// The interrupt routine - runs each time a falling edge of a pulse is detected
void onPulse()                  
{
  pulseTime = lastPulseTime;        			//used to measure time between pulses.
  pulseTime = micros();
  pulseCount++;                                                      //pulseCounter               
  //power = int((3600000000.0 / (pulseTime - lastTime))/ppwh);  //Estimated power calculation 
  emonPi.pulse_elapsedkWh = (1.0*pulseCount/(ppwh*1000)); 	//multiply by 1000 to pulses per wh to kwh convert wh to kwh
}
#line 1 "/home/glyn/Dropbox/Energy Monitoring/Open Energy Monitor/emonTx/emonPi/Software/emonpi/Atmega328/emonPi_RFM69CW_RF12Demo_DiscreteSampling/emonPi_RF.ino"
void RF_Setup(){
	//--------------------------------------------------Initalize RF and send out RF test packets--------------------------------------------------------------------------------------------  
  //NEED TO ADD RF MODULE AUTO DETECTION 
  delay(10);
  rf12_initialize(nodeID, RF_freq, networkGroup);                          // initialize RFM12B/rfm69CW
   for (int i=10; i>=0; i--)                                                                  //Send RF test sequence (for factory testing)
   {
     emonPi.power1=i; 
     rf12_sendNow(0, &emonPi, sizeof emonPi);
     delay(100);
   }
  rf12_sendWait(2);
  emonPi.power1=0;
 //-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
}

boolean RF_Rx_Handle(){

	if (rf12_recvDone()) {						//if RF Packet is received 
	    byte n = rf12_len;
	    if (rf12_crc == 0)							//Check packet is good
	    {
	    	Serial.print("OK");
	    	Serial.print(" ");							//Print RF packet to serial in struct format
	    	Serial.print(rf12_hdr & 0x1F);				// Extract and print node ID
	    	Serial.print(" ");
	    	for (byte i = 0; i < n; ++i) {
	      		Serial.print((word)rf12_data[i]);
	      		Serial.print(' ');
	    	}

	      	#if RF69_COMPAT
		    // display RSSI value after packet data e.g (-xx)
		    Serial.print("(");
		    Serial.print(-(RF69::rssi>>1));
		    Serial.print(")");
			#endif
		    	Serial.println();

	        if (RF12_WANTS_ACK==1) {
	           Serial.print(" -> ack");
	           rf12_sendStart(RF12_ACK_REPLY, 0, 0);
	       }

	    return(1);
	    }
	    else
			return(0);
	       
	} //end recDone
	
}

void send_RF(){

	if (cmd && rf12_canSend() ) {                                                //if command 'cmd' is waiting to be sent then let's send it
	    digitalWrite(LEDpin, HIGH); delay(200); digitalWrite(LEDpin, LOW);
	    showString(PSTR(" -> "));
	    Serial.print((word) sendLen);
	    showString(PSTR(" b\n"));
	    byte header = cmd == 'a' ? RF12_HDR_ACK : 0;
	    if (dest)
	      header |= RF12_HDR_DST | dest;
	    rf12_sendStart(header, stack, sendLen);
	    cmd = 0;
	    
	}
}


static void handleInput (char c) {
  if ('0' <= c && c <= '9') {
    value = 10 * value + c - '0';
    return;
  }

  if (c == ',') {
    if (top < sizeof stack)
      stack[top++] = value; // truncated to 8 bits
    value = 0;
    return;
  }

  if (c > ' ') {

    switch (c) {

      case 'i': //set node ID
        if (value){
          nodeID = value;
          if (RF_STATUS==1) rf12_initialize(nodeID, RF_freq, networkGroup);
        break;
      }

      case 'b': // set band: 4 = 433, 8 = 868, 9 = 915
        value = bandToFreq(value);
        if (value){
          RF_freq = value;
          if (RF_STATUS==1) rf12_initialize(nodeID, RF_freq, networkGroup);
        }
        break;
    
      case 'g': // set network group
        if (value){
          networkGroup = value;
          if (RF_STATUS==1) rf12_initialize(nodeID, RF_freq, networkGroup);
        }
          break;

      case 'v': // set Vcc Cal 1=UK/EU 2=USA
        if (value){
          if (value==1) USA=false;
          if (value==2) USA=true;
        }
          break;

      case 'a': // send packet to node ID N, request an ack
      case 's': // send packet to node ID N, no ack
        cmd = c;
        sendLen = top;
        dest = value;
        break;

        default:
          showString(helpText1);
      } //end case 
    //Print Current RF config  
    if (RF_STATUS==1) {
      Serial.print(' ');
      Serial.print((char) ('@' + (nodeID & RF12_HDR_MASK)));
      Serial.print(" i");
      Serial.print(nodeID & RF12_HDR_MASK);   
      Serial.print(" g");
      Serial.print(networkGroup);
      Serial.print(" @ ");
      Serial.print(RF_freq == RF12_433MHZ ? 433 :
                   RF_freq == RF12_868MHZ ? 868 :
                   RF_freq == RF12_915MHZ ? 915 : 0);
      Serial.print(" MHz"); 
      Serial.print(" USA "); Serial.print(USA);
      Serial.println(" ");

    }
    
    }
  value = top = 0;
}


static byte bandToFreq (byte band) {
  return band == 4 ? RF12_433MHZ : band == 8 ? RF12_868MHZ : band == 9 ? RF12_915MHZ : 0;
}
 

#line 1 "/home/glyn/Dropbox/Energy Monitoring/Open Energy Monitor/emonTx/emonPi/Software/emonpi/Atmega328/emonPi_RFM69CW_RF12Demo_DiscreteSampling/emonPi_Serial_LCD.ino"
// emonPi used 16 x 2 I2C LCD display 

void emonPi_LCD_Startup() {
  lcd.init();                      // initialize the lcd 
  lcd.backlight();                 // Or lcd.noBacklight() 
  lcd.print("emonPi V"); lcd.print(firmware_version);
  lcd.setCursor(0, 1); lcd.print("OpenEnergyMon");
  delay(2000);
    lcd.setCursor(0, 1); lcd.print("Detecting CT's.."); 
} 

void serial_print_startup(){
  //-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  lcd.clear(); 

  Serial.print("CT 1 Cal: "); Serial.println(Ical1);
  Serial.print("CT 2 Cal: "); Serial.println(Ical2);
  delay(1000);

  Serial.print("VRMS AC ~");
  Serial.print(vrms,0); Serial.println("V");

  if (ACAC) 
  {
    lcd.print("AC Wave Detected");
    Serial.println("AC Wave Detected - Real Power calc enabled");
    if (USA==TRUE) Serial.println("USA mode"); 
    Serial.print("Vcal: "); Serial.println(Vcal);
    Serial.print("Phase Shift: "); Serial.println(phase_shift);
  }
  else 
  {
    lcd.print("AC NOT Detected");
   Serial.println("AC NOT detected - Apparent Power calc enabled");
   Serial.print("Assuming VRMS to be "); Serial.print(Vrms); Serial.println("V");
 }  

lcd.setCursor(0, 1); lcd.print("Detected ");

  if (CT_count==0) {
    Serial.println("NO CT detected, sampling from CT1 anyway");
    lcd.print("No CT's");
  }
   else   
   {
    
     if (CT1) {
      Serial.println("CT 1 detected");
      lcd.print("CT1 ");
    }
     if (CT2) {
      Serial.println("CT 2 detected");
      lcd.print("CT2");
    }
   }
  if (DS18B20_STATUS==1) {
    Serial.print("Detected "); 
    Serial.print(numSensors); 
    Serial.println(" DS18B20");
    delay(5000);
    lcd.clear();
    lcd.print("Detected: "); lcd.print(numSensors); 
    lcd.setCursor(0, 1); lcd.print("DS18B20 Temp"); 
  }
  else Serial.println("Zero DS18B20 temp sensor detected");

  if (RF_STATUS == 1){
    #if (RF69_COMPAT)
      Serial.println("RFM69CW Init: ");
    #else
      Serial.println("RFM12B Init: ");
    #endif

    Serial.print("Node "); Serial.print(nodeID); 
    Serial.print(" Freq "); 
    if (RF_freq == RF12_433MHZ) Serial.print("433Mhz");
    if (RF_freq == RF12_868MHZ) Serial.print("868Mhz");
    if (RF_freq == RF12_915MHZ) Serial.print("915Mhz"); 
    Serial.print(" Network "); Serial.println(networkGroup);
  }
  delay(500);  
}


void send_emonpi_serial()  //Send emonPi data to Pi serial /dev/ttyAMA0 using struct JeeLabs RF12 packet structure 
{
  byte binarray[sizeof(emonPi)];
  memcpy(binarray, &emonPi, sizeof(emonPi));
  
  Serial.print("OK ");
  Serial.print(nodeID);
  for (byte i = 0; i < sizeof(binarray); i++) {
    Serial.print(' ');
    Serial.print(binarray[i]);
  }
  Serial.print(" (-0)");
  Serial.println();
  
  delay(10);
}

static void showString (PGM_P s) {
  for (;;) {
    char c = pgm_read_byte(s++);
    if (c == 0)
      break;
    if (c == '\n')
      Serial.print('\r');
    Serial.print(c);
  }
}
#line 1 "/home/glyn/Dropbox/Energy Monitoring/Open Energy Monitor/emonTx/emonPi/Software/emonpi/Atmega328/emonPi_RFM69CW_RF12Demo_DiscreteSampling/emonPi_Startup.ino"
 
double calc_rms(int pin, int samples)                      //Used in emonPi startup to detect presence of AC wavefrom and estimate VRMS voltage
{
  unsigned long sum = 0;
  for (int i=0; i<samples; i++) // 178 samples takes about 20ms
  {
    int raw = (analogRead(0)-512);
    sum += (unsigned long)raw * raw;
  }
  double rms = sqrt((double)sum / samples);
  return rms;
}

void emonPi_startup()                                                     //emonPi startup proceadure, check for AC waveform and print out debug
{
  pinMode(LEDpin, OUTPUT); 
  digitalWrite(LEDpin,HIGH); 

  pinMode(shutdown_switch_pin,INPUT_PULLUP);            //enable ATmega328 internal pull-up resistors 

  pinMode(emonpi_GPIO_pin, OUTPUT);                     //Connected to RasPi GPIO pin 17
  digitalWrite(emonpi_GPIO_pin, LOW);

  Serial.begin(BAUD_RATE);
  Serial.print("emonPi V"); Serial.print(firmware_version); 
  Serial.println("OpenEnergyMonitor.org");
  Serial.println("POST.....wait 10s");
}


 
void CT_Detect(){
//--------------------------------------------------Check for connected CT sensors--------------------------------------------------------------------------------------------------------- 
if (analogRead(1) > 0) {CT1 = 1; CT_count++;} else CT1=0;              // check to see if CT is connected to CT1 input, if so enable that channel
if (analogRead(2) > 0) {CT2 = 1; CT_count++;} else CT2=0;              // check to see if CT is connected to CT2 input, if so enable that channel
if ( CT_count == 0) CT1=1;                                                                        // If no CT's are connected then by default read from CT1
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//--------------------------------------------------Check for connected AC Adapter Sensor------------------------------------------------------------------------------------------------

// Quick check to see if there is a voltage waveform present on the ACAC Voltage input
// Check consists of calculating the RMS from 100 samples of the voltage input.
Sleepy::loseSomeTime(10000);            //wait for settle
digitalWrite(LEDpin,LOW); 

// Calculate if there is an ACAC adapter on analog input 0
vrms = calc_rms(0,1780) * 0.87;      //ADC 0   double vrms = calc_rms(0,1780) * (Vcal * (3.3/1024) );
if (vrms>90) ACAC = 1; else ACAC=0;
//Serial.print(vrms);

if (ACAC) 
{
  for (int i=0; i<10; i++)                                              // indicate AC has been detected by flashing LED 10 times
  { 
    digitalWrite(LEDpin, HIGH); delay(200);
    digitalWrite(LEDpin, LOW); delay(300);
  }
}
else 
{
  delay(1000);
  digitalWrite(LEDpin, HIGH); delay(2000); digitalWrite(LEDpin, LOW);   // indicate that no AC signal has been detected by turing LED on then off
}

}



#line 1 "/home/glyn/Dropbox/Energy Monitoring/Open Energy Monitor/emonTx/emonPi/Software/emonpi/Atmega328/emonPi_RFM69CW_RF12Demo_DiscreteSampling/emonPi_temperature.ino"

byte check_for_DS18B20()                                      //Setup and for presence of DS18B20, return number of sensors 
{
  sensors.begin();
  numSensors=(sensors.getDeviceCount()); 
  
  byte j=0;                                        // search for one wire devices and
                                                   // copy to device address arrays.
  while ((j < numSensors) && (oneWire.search(allAddress[j])))  j++;
  for(byte j=0;j<numSensors;j++) sensors.setResolution(allAddress[j], TEMPERATURE_PRECISION);      // and set the a to d conversion resolution of each.
  
  if (numSensors==0) DS18B20_STATUS=0; 
    else DS18B20_STATUS=1;

  if (numSensors>MaxOnewire) numSensors=MaxOnewire;     //If more sensors are detected than allowed only read from max allowed number
    
 return numSensors;   
}

int get_temperature(byte sensor)                
{
    {
     sensors.requestTemperatures();                                        // Send the command to get temperatures
     float temp=(sensors.getTempC(allAddress[sensor]));
   
     if ((temp<125.0) && (temp>-40.0)) return(temp*10);            //if reading is within range for the sensor convert float to int ready to send via RF
     if (debug==1) {Serial.print("temp "); Serial.print(sensor); Serial.print(" "); Serial.println(temp);}
  }
}


