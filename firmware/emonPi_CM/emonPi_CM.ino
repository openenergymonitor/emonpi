/*


  emonPi Continuous Monitoring - radio using JeeLib RFM69 "Native" format

  
   ------------------------------------------
  Part of the openenergymonitor.org project

  Authors: Glyn Hudson, Trystan Lea & Robert Wall
  Builds upon JCW JeeLabs RF69 Driver and Arduino

  Licence: GNU GPL V3

//----------------------------emonPi Firmware Version---------------------------------------------------------------------------------------- 
*/

const byte firmware_version[3] = {1,1,3};
/*
V1.0.0   10/7/2021 Derived from emonLibCM examples and original emonPi sketch, that being derived from 
            https://github.com/openenergymonitor/emonpi/blob/master/Atmega328/emonPi_RFM69CW_RF12Demo_DiscreteSampling
            and emonLibCM example sketches, with config input based on emonTx V3 sketches.
v1.1.0   16/2/2023 Support for LowPowerLabs
v1.1.1   16/2/2023 Print Radio format at startup, include message count in output
v1.1.2   28/3/2023 Fix missing ACKRequested sendACK
v1.1.3   04/4/2023 Updated to use cut down version of RFM69 LowPowerLabs library
                   Updated to match latest EmonLibCM library and emonPiFrontEndCM sketch from Robert

emonhub.conf node decoder (assuming Node 5):

[[5]]
    nodename = emonpi
    [[[rx]]]
        names = Msg, power1,power2,power1pluspower2,vrms,t1,t2,t3,t4,t5,t6,pulse1count,pulse2count,E1,E2
        datacodes = L, h, h, h, h, h, h, h, h, h, h, L, L, l, l
        scales = 1, 1,1,1, 0.01, 0.01,0.01,0.01,0.01,0.01,0.01, 1, 1, 1,1
        units = n,W,W,W, V, C,C,C,C,C,C, p, p, Wh,Wh

*/
// #define EEWL_DEBUG
// #define SAMPPIN 19 

#define RFM69_JEELIB_NATIVE 2
#define RFM69_LOW_POWER_LABS 3

#define RadioFormat RFM69_LOW_POWER_LABS

#include <emonLibCM.h>                                                 // OEM CM library

#include <emonEProm.h>                                                // OEM EPROM library

// RFM interface
#if RadioFormat == RFM69_LOW_POWER_LABS
  #include <RFM69_LPL.h>
  RFM69 radio;
#else
  #include "spi.h"                                                       // Requires "RFM69 Native" JeeLib Driver
  #include "rf69.h"
  RF69<SpiDev10> rf;
#endif

bool rfDataAvailable = false;

byte nativeMsg[66];                                                    // 'Native' format message buffer

#define MAXMSG 62                                                      // Max length of o/g message - payload can be 62 bytes max in RFM69
char outmsg[MAXMSG];                                                   // outgoing message (to emonGLCD etc)
byte outmsgLength;                                                     // length of message: non-zero length triggers transmission
byte txDestId = 0 ;                                                    // 
struct {                                                               // Ancilliary information
  byte srcNode = 0;
  byte msgLength = 0;
  signed char rssi = -127;
  bool crc = false;
} rfInfo;

enum rfband {RFM_433MHZ = 1, RFM_868MHZ, RFM_915MHZ };                 // frequency band.
bool rfChanged = false;                                                // marker to re-initialise radio
#define RFRX 0x01                                                      // Control of RFM - receive enabled
#define RFTX 0x02                                                      // Control of RFM - transmit enabled

#include <LiquidCrystal_I2C.h>                                         // https://github.com/openenergymonitor/LiquidCrystal_I2C

int i2c_lcd_address[2]          = {0x27, 0x3f};                        // I2C addresses to test for I2C LCD device
int current_lcd_i2c_addr;                                              // Used to store current I2C address as found by i2_lcd_detect()
// LiquidCrystal_I2C lcd(0x27,16,2);                                   // Placeholder
LiquidCrystal_I2C lcd(0,0,0);

bool backlight = false;                                                // Controls for backlight timeout
bool backlightTimer = false;
unsigned long backlightOn;
#define BACKLIGHT_TIMEOUT 60000UL                                      // Timeout in ms (only for data from here)


//----------------------------emonPi Settings------------------------------------------------------------------------------------------------
bool debug                      = true;
bool verbose                    = false;
const unsigned long BAUD_RATE   = 38400;


int i2c_LCD_Detect(int i2c_lcd_address[]);
void emonPi_LCD_Startup(int current_i2c_addr);
void Startup_to_LCD(int current_lcd_i2c_addr);
void lcd_print_currents(int current_lcd_i2c_addr, float I1, float I2, float pf1, float pf2);
void single_LED_flash(void);
void double_LED_flash(void);
void getCalibration(void);
void send_emonpi_serial();
void emonPi_startup(void);
static void showString (PGM_P s);


//--------------------------- temperature data ----------------------------------------------------------------------------------------------

const byte MaxOnewire = 6;                              // maximum number of DS18B20 one wire sensors 
int allTemps[MaxOnewire];                               // Array to receive temperature measurements
const byte TEMPERATURE_PRECISION = 11;                  // 9 (93.8ms), 10 (187.5ms), 11 (375ms) or 12 (750ms) bits equal to resolution of 0.5C, 0.25C, 0.125C and 0.0625C

//---------------------------- emonPi Settings - Stored in EEPROM and shared with config.ino ------------------------------------------------
struct {
  byte RF_freq = RFM_433MHZ;                            // Frequency of radio module can be RFM_433MHZ, RFM_868MHZ or RFM_915MHZ. 
  byte rfPower = 25;                                    // Power when transmitting
  byte networkGroup = 210;                              // wireless network group, must be the same as emonBase / emonPi and emonGLCD. OEM default is 210
  byte  nodeID = 5;                                     // node ID for this emonPi.
  float vCal  = 268.97;                                 // (240V x 13) / 11.6V = 268.97 Calibration for UK AC-AC adapter 77DB-06-09
  float assumedVrms = 240.0;                            // Assumed Vrms when no a.c. is detected
  float lineFreq = 50;                                  // Line Frequency = 50 Hz
  float i1Cal = 90.9;                                   // (100 A / 50 mA / 22 Ohm burden) = 90.9
  float i1Lead = 1.2;                                   // 1.2° phase lead
  float i2Cal = 90.9;
  float i2Lead = 1.2; 
  float period = 9.85;                                  // datalogging period - should be fractionally less than the PHPFINA database period in emonCMS
  bool  pulse_enable = true;                            // pulse 1 counting 1
  int   pulse_period = 0;                               // pulse 1 min period - 0 = no de-bounce
  bool  pulse2_enable = false;                          // pulse 2 counting 2
  int   pulse2_period = 0;                              // pulse 2 min period - 0 = no de-bounce
  bool  temp_enable = true;                             // enable temperature measurement
  DeviceAddress allAddresses[MaxOnewire];               // sensor address data
  bool  showCurrents = false;                           // Print to serial voltage, current & p.f. values  
  byte  rfOn = 0x03;                                    // Turn transmitter AND receiver on
} EEProm;

uint16_t eepromSig = 0x0010;                            // EEPROM signature - see oemEProm Library documentation for details.

//-------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------

bool shutdown_switch_last_state = false;

//--------------------------- hard-wired connections ----------------------------------------------------------------------------------------

const byte LEDpin               = 9;                                   // emonPi LED - on when HIGH
const byte shutdown_switch_pin  = 8;                                   // Push-to-make - Low when pressed
const byte emonpi_GPIO_pin      = 5;                                   // Connected to Pi GPIO 17, used to activate Pi Shutdown when HIGH
//const byte emonPi_RJ45_8_IO   = A6;                                  // RJ45 pin 8 - Analog 6 - Aux I/O - D20?
const byte emonPi_int0          = 0;                                   // RJ45 pin 6 - INT0 - Dig 2 - default pulse count interrupt
const byte emonPi_int0_pin      = 2;                                   // RJ45 pin 6 - INT0 - Dig 2 - default pulse count input pin
const byte emonPi_int1          = 1;                                   // RJ45 pin 6 - INT1 - Dig 3 - default pulse count interrupt
const byte emonPi_int1_pin      = 3;                                   // RJ45 pin 6 - INT1 - Dig 3 - default pulse count input pin

// Use D5 for ISR timimg checks - only if Pi is not connected!

//-------------------------------------------------------------------------------------------------------------------------------------------

struct 
{
  unsigned long Msg;
  int power1;                                                          // Powers in watts
  int power2;
  int power1_plus_2;                                                   // added here to assure sum is from the same sampling period
  int Vrms;
  int temp[MaxOnewire];
  unsigned long pulseCount;                                            // in whatever units the meter records
  unsigned long pulse2Count;                                           // in whatever units the meter records
  long E1;                                                             // Accumulated energy from powers in Wh
  long E2;
} emonPi;                                                              // Data format for byte-wise serial transfer to Raspberry Pi
//-------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------

#ifdef EEWL_DEBUG
  extern EEWL EVmem;
#endif


/**************************************************************************************************************************************************
*
* SETUP        Set up & start the display, the radio & emonLibCM
*
***************************************************************************************************************************************************/
void setup() 
{  

  emonPi_startup();

  delay(2000);

  EmonLibCM_SetADC_VChannel(0, EEProm.vCal);                           // ADC Input channel, voltage calibration - for Ideal UK Adapter = 268.97
  EmonLibCM_SetADC_IChannel(1, EEProm.i1Cal, EEProm.i1Lead);           // ADC Input channel, current calibration, phase calibration
  EmonLibCM_SetADC_IChannel(2, EEProm.i2Cal, EEProm.i2Lead);           // The current channels will be read in this order

  EmonLibCM_ADCCal(3.3);                                               // ADC Reference voltage, (3.3 V)
  EmonLibCM_cycles_per_second(EEProm.lineFreq);                        // mains frequency 50/60Hz
  
  EmonLibCM_setAssumedVrms(EEProm.assumedVrms);

  EmonLibCM_setPulseEnable(true);                                      // Enable pulse counting
  EmonLibCM_setPulsePin(0, emonPi_int1_pin, emonPi_int1);              // Pulse input Pin, Interrupt
  EmonLibCM_setPulseMinPeriod(0, EEProm.pulse_period, (byte)FALLING);  // Minimum pulse period
  EmonLibCM_setPulseEnable(1, EEProm.pulse2_enable);                   // Enable pulse counting
  EmonLibCM_setPulsePin(1, emonPi_int1_pin, emonPi_int1);              // Pulse input Pin, Interrupt
  EmonLibCM_setPulseMinPeriod(1, EEProm.pulse_period, (byte)FALLING);  // Minimum pulse period

  EmonLibCM_setTemperatureDataPin(4);                                  // OneWire data pin 
  EmonLibCM_setTemperaturePowerPin(-1);                                // Temperature sensor Power Pin - Not used.
  EmonLibCM_setTemperatureResolution(TEMPERATURE_PRECISION);           // Resolution in bits, allowed values 9 - 12. 11-bit resolution, reads to 0.125 degC
  EmonLibCM_setTemperatureAddresses(EEProm.allAddresses, true);        // Name of array of temperature sensors
  EmonLibCM_setTemperatureArray(allTemps);                             // Name of array to receive temperature measurements
  EmonLibCM_setTemperatureMaxCount(MaxOnewire);                        // Max number of sensors, limited by wiring and array size.


#ifdef EEWL_DEBUG
  Serial.print(F("End of mem="));Serial.print(E2END);
  Serial.print(F(" Avail mem="));Serial.print((E2END>>2) * 3);
  Serial.print(F(" Start addr="));Serial.print(E2END - (((E2END>>2) * 3) / (sizeof(mem)+1))*(sizeof(mem)+1));
  Serial.print(F(" Num blocks="));Serial.println(((E2END>>2) * 3) / 21);
  EVmem.dump_buffer();
#endif
  // Recover saved energy & pulse count from EEPROM
  {
    long E1 = 0, E2 = 0;
    unsigned long p1 = 0, p2 = 0;
    
    recoverEValues(&E1, &E2, &p1, &p2);
    EmonLibCM_setWattHour(0, E1);
    EmonLibCM_setWattHour(1, E2);
    EmonLibCM_setPulseCount(0, p1);
    EmonLibCM_setPulseCount(1, p2);
  }

  EmonLibCM_TemperatureEnable(EEProm.temp_enable);                     // Temperature monitoring enable
  EmonLibCM_min_startup_cycles(10);                                    // number of cycles to let ADC run before starting first actual measurement

  EmonLibCM_datalog_period(0.5);                                       // Get a quick reading for display of voltage, temp sensor count.
  

  // Detect and startup I2C LCD
  current_lcd_i2c_addr = i2c_LCD_Detect(i2c_lcd_address);

  LiquidCrystal_I2C lcd(current_lcd_i2c_addr,16,2);                    // LCD I2C address to 0x27, 16x2 line display
  emonPi_LCD_Startup(current_lcd_i2c_addr);
  backlightTimer = true;

  EmonLibCM_Init();                                                    // Start continuous monitoring.

  while (!EmonLibCM_Ready())                                           // Quick check for presence of a voltage
    ;
 
  EmonLibCM_datalog_period(EEProm.period);                             // Reset to NORMAL period of readings in seconds for emonCMS
  Startup_to_LCD(current_lcd_i2c_addr);

  #if RadioFormat == RFM69_LOW_POWER_LABS
    radio.initialize(RF69_433MHZ,EEProm.nodeID,EEProm.networkGroup);  
    radio.encrypt("89txbe4p8aik5kt3");
  #else
    rf.init(EEProm.nodeID, EEProm.networkGroup, 
               EEProm.RF_freq == RFM_915MHZ ? 915                      // Fall through to 433 MHz Band @ 434 MHz
            : (EEProm.RF_freq == RFM_868MHZ ? 868 : 434)); 
  #endif
  
  emonPi.Msg = 0;
}

/**************************************************************************************************************************************************
*
* LOOP         Read the pushbutton, poll the radio for incoming data, the serial input for calibration & emonLibCM for energy readings
*
***************************************************************************************************************************************************/

void loop()             
{
  
  if (digitalRead(shutdown_switch_pin) == 0 )
  {
    digitalWrite(emonpi_GPIO_pin, HIGH);                               // if emonPi shutdown button pressed then send signal to the Pi on GPIO 11
    shutdown_switch_last_state = true;
  }
  else
  {
    digitalWrite(emonpi_GPIO_pin, LOW);
    if (shutdown_switch_last_state)
      asm volatile ("  jmp 0");

  }
  
//-------------------------------------------------------------------------------------------------------------------------------------------
// RF Data handler - inbound ****************************************************************************************************************
//-------------------------------------------------------------------------------------------------------------------------------------------

  if ((EEProm.rfOn & RFRX))
  {
    #if RadioFormat == RFM69_LOW_POWER_LABS
    if (radio.receiveDone())
    {    
      rfInfo.msgLength = radio.DATALEN;
      rfInfo.srcNode = radio.SENDERID;
      for (byte i = 0; i < radio.DATALEN; i++) {
        nativeMsg[i] = radio.DATA[i];
      }
      rfInfo.rssi = radio.readRSSI();
      
      if (radio.ACKRequested()) {
        radio.sendACK();
      }    
    
      // send serial data
      Serial.print(F("OK"));                                              // Bad packets (crc failure) are discarded by RFM69CW
      print_frame(rfInfo.msgLength);		                                  // Print received data
      double_LED_flash();
    }
    #else
    int len = rf.receive(&nativeMsg, sizeof(nativeMsg));                 // Poll the RFM buffer and extract the data
    if (len > 1)
    {
      rfInfo.crc = true;
      rfInfo.msgLength = len;
      rfInfo.srcNode = nativeMsg[1];
      rfInfo.rssi = -rf.rssi/2;

      // send serial data
      Serial.print(F("OK"));                                              // Bad packets (crc failure) are discarded by RFM69CW
      print_frame(rfInfo.msgLength);		                                  // Print received data
      double_LED_flash();
    }
    #endif
  } 
  
//-------------------------------------------------------------------------------------------------------------------------------------------
// RF Data handler - outbound ***************************************************************************************************************
//-------------------------------------------------------------------------------------------------------------------------------------------


	if ((EEProm.rfOn & RFTX) && outmsgLength) {                           //if command 'outmsg' is waiting to be sent then let's send it
    digitalWrite(LEDpin, HIGH); delay(200); digitalWrite(LEDpin, LOW);
    Serial.print ("Sending ") ; Serial.print((word) outmsgLength); Serial.print(" bytes "); Serial.print("to node " ); Serial.println(txDestId) ;
    #if RadioFormat == RFM69_LOW_POWER_LABS
      radio.send(0, (void *)outmsg, outmsgLength);
    #else
      rf.send(txDestId, (void *)outmsg, outmsgLength);                    //  void RF69<SPI>::send (uint8_t header, const void* ptr, int len) {
    #endif
    outmsgLength = 0;
	}

  
//-------------------------------------------------------------------------------------------------------------------------------------------
// Calibration Data handler *****************************************************************************************************************
//-------------------------------------------------------------------------------------------------------------------------------------------

  if (Serial.available())                                              // Serial input from RPi for configuration/calibration
  {
    getCalibration();                                                  // If serial input is received from RPi
    double_LED_flash();
    if (rfChanged)
    {
      #if RadioFormat == RFM69_LOW_POWER_LABS
        radio.initialize(EEProm.RF_freq,EEProm.nodeID,EEProm.networkGroup);  
        radio.encrypt("89txbe4p8aik5kt3"); 
      #else
        rf.init(EEProm.nodeID, EEProm.networkGroup,                      // Reset the RFM69CW if NodeID, Group or frequency has changed.
            EEProm.RF_freq == RFM_915MHZ ? 915 : (EEProm.RF_freq == RFM_868MHZ ? 868 : 434)); 
      #endif
      rfChanged = false;
    }
  }

//-------------------------------------------------------------------------------------------------------------------------------------------
// Energy Monitor ***************************************************************************************************************************
//-------------------------------------------------------------------------------------------------------------------------------------------

  if (EmonLibCM_Ready())   
  {
    single_LED_flash();                                                // single flash of LED on local CT sample

    emonPi.power1 = EmonLibCM_getRealPower(0);                       // Copy the desired variables ready for transmission 
    emonPi.power2 = EmonLibCM_getRealPower(1); 
    emonPi.power1_plus_2 = emonPi.power1 + emonPi.power2;
    emonPi.E1     = EmonLibCM_getWattHour(0);
    emonPi.E2     = EmonLibCM_getWattHour(1);
   
    emonPi.Vrms     = EmonLibCM_getVrms() * 100;                       // Always send the ACTUAL measured voltage.

    if (EmonLibCM_getTemperatureSensorCount())
    {
      for (byte i=0; i< MaxOnewire; i++)
        emonPi.temp[i] = allTemps[i];
    }
   
    emonPi.pulseCount = EmonLibCM_getPulseCount(0);
    emonPi.pulse2Count = EmonLibCM_getPulseCount(1);
    
    send_emonpi_serial();                                              // Send emonPi data to Pi serial using packet structure

    if (EEProm.showCurrents)
    {
      // to show voltage, current & power factor for calibration:
      Serial.print(F("|Vrms:")); Serial.print(EmonLibCM_getVrms());

      Serial.print(F(", I1:")); Serial.print(EmonLibCM_getIrms(0));
      Serial.print(F(", pf1:")); Serial.print(EmonLibCM_getPF(0),4);

      Serial.print(F(", I2:")); Serial.print(EmonLibCM_getIrms(1));
      Serial.print(F(", pf2:")); Serial.print(EmonLibCM_getPF(1),4);
      
      Serial.print(F(", f:")); Serial.print(EmonLibCM_getLineFrequency());
      Serial.println();
      
      lcd_print_currents(current_lcd_i2c_addr, EmonLibCM_getIrms(0), EmonLibCM_getIrms(1), EmonLibCM_getPF(0), EmonLibCM_getPF(1));
   }
    delay(50);

    // Save energy & pulse count values to EEPROM
    storeEValues(emonPi.E1, emonPi.E2, emonPi.pulseCount, emonPi.pulse2Count);
  }

//-------------------------------------------------------------------------------------------------------------------------------------------
// Backlight (also controlled from Pi) ******************************************************************************************************
//-------------------------------------------------------------------------------------------------------------------------------------------

  if (backlightTimer)
  {
    backlightOn = millis();
    backlight = true;
    backlightTimer = false;
  }
  if (backlight && (millis() - backlightOn) > BACKLIGHT_TIMEOUT)
  {
    LiquidCrystal_I2C lcd(current_lcd_i2c_addr,16,2);
    lcd.noBacklight();                                                   // It can get left on if this is restarted without the Pi rebooting    
    backlight = false;
  }
}

/**************************************************************************************************************************************************
*
* SEND RECEIVED RF DATA TO RPi (/dev/ttyAMA0)
*
***************************************************************************************************************************************************/

void print_frame(int len) 
{
  Serial.print(F(" "));
  Serial.print(rfInfo.srcNode);        // Extract and print node ID
  Serial.print(F(" "));
  for (byte i = 0; i < len; ++i) 
  {
    Serial.print((word)nativeMsg[i]);
    Serial.print(F(" "));
  }
  Serial.print(F("("));
  Serial.print(rfInfo.rssi);
  Serial.print(F(")"));
  Serial.println();
}

/**************************************************************************************************************************************************
*
* LED flash
*
***************************************************************************************************************************************************/

void single_LED_flash(void)
{
  digitalWrite(LEDpin, HIGH);  delay(30); digitalWrite(LEDpin, LOW);
}

void double_LED_flash(void)
{
  digitalWrite(LEDpin, HIGH);  delay(20); digitalWrite(LEDpin, LOW); delay(60); 
  digitalWrite(LEDpin, HIGH);  delay(20); digitalWrite(LEDpin, LOW);
}


/**************************************************************************************************************************************************
*
* Startup      Set I/O pins, print initial message, read configuration from EEPROM
*
***************************************************************************************************************************************************/

void emonPi_startup()
{
  pinMode(LEDpin, OUTPUT);
  digitalWrite(LEDpin,HIGH);

  pinMode(shutdown_switch_pin,INPUT_PULLUP);                           // enable ATmega328 internal pull-up resistors

  pinMode(emonpi_GPIO_pin, OUTPUT);                                    // Connected to RasPi GPIO pin 17
  digitalWrite(emonpi_GPIO_pin, LOW);

  Serial.begin(BAUD_RATE);
  Serial.print(F("|emonPiCM V")); printVersion();
  Serial.println(F("|OpenEnergyMonitor.org"));

  #if RadioFormat == RFM69_LOW_POWER_LABS
    Serial.println(F("|Radio format: LowPowerLabs"));
  #else
    Serial.println(F("|Radio format: JeeLib Native"));
  #endif

  load_config();                                                      // Load RF config from EEPROM (if any exists)

#ifdef SAMPPIN
  pinMode(SAMPPIN, OUTPUT);
  digitalWrite(SAMPPIN, LOW);
#endif
  
}


/***************************************************************************************************************************************************
*
* lcd - I²C   Set up 16 x 2 I²C LCD display, get startup data & print startup message, print currents as required
*
***************************************************************************************************************************************************/

int i2c_LCD_Detect(int i2c_lcd_address[])
{
  Wire.begin();
  byte error=1;
  for (int i=0; i<2; i++)
  {
    Wire.beginTransmission(i2c_lcd_address[i]);
    error = Wire.endTransmission();
    if (error == 0)
    {
      Serial.print(F("LCD found i2c 0x")); Serial.println(i2c_lcd_address[i], HEX);
      return (i2c_lcd_address[i]);
      break;
    }
  }
  Serial.println(F("LCD not found"));
  return(0);
}


void emonPi_LCD_Startup(int current_i2c_addr) 
{
  LiquidCrystal_I2C lcd(current_i2c_addr,16,2);                        // LCD I2C address to 0x27, 16x2 line display
  lcd.init();                      // initialize the lcd
  lcd.backlight();                 // Or lcd.noBacklight()
  lcd.print(F("emonPi V"));
  lcd.print(firmware_version[0]);
  lcd.print(".");
  lcd.print(firmware_version[1]);
  lcd.print(".");
  lcd.print(firmware_version[2]);
  lcd.setCursor(0, 1); lcd.print(F("OpenEnergyMon"));
  delay(2000);
  backlightTimer = true;
}

void Startup_to_LCD(int current_lcd_i2c_addr)                          // Print to LCD
{
  //-----------------------------------------------------------------------------------------------------------------------------------------------
  LiquidCrystal_I2C lcd(current_lcd_i2c_addr,16,2);                    // LCD I2C address to 0x27, 16x2 line display
  lcd.clear();
  lcd.backlight();

  if (EmonLibCM_acPresent())
    lcd.print(F("AC Wave Detected"));
  else
    lcd.print(F("AC NOT Detected"));
  delay(4000);


  lcd.clear();
  lcd.print(F("Detected: ")); lcd.print(EmonLibCM_getTemperatureSensorCount());
  lcd.setCursor(0, 1); lcd.print(F("DS18B20 Temp"));
  delay(2000);

  lcd.clear();
  lcd.print(F("Pi is booting..."));
  lcd.setCursor(0, 1); lcd.print(F("Please wait"));
  backlightTimer = true;
  delay(20);
}


void lcd_print_currents(int current_lcd_i2c_addr, float I1, float I2, float pf1, float pf2)
{
  //-----------------------------------------------------------------------------------------------------------------------------------------------
  LiquidCrystal_I2C lcd(current_lcd_i2c_addr,16,2);                    // LCD I2C address to 0x27, 16x2 line display
  lcd.clear();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print(F("1 "));lcd.print(I1);lcd.print(F(" "));lcd.print(pf1,4);
  lcd.setCursor(0, 1);
  lcd.print(F("2 "));lcd.print(I2);lcd.print(F(" "));lcd.print(pf2,4);
  backlightTimer = true;
}


/**************************************************************************************************************************************************
*
* SEND OWN SERIAL DATA TO RPi (/dev/ttyAMA0) using packet structure
*
***************************************************************************************************************************************************/

void send_emonpi_serial()
{
  emonPi.Msg++;
  byte *b = (byte *)&emonPi;

  Serial.print(F("OK "));
  Serial.print(EEProm.nodeID);
  
  for (byte i = 0; i < sizeof(emonPi); i++) 
  {
    Serial.print(F(" "));
    Serial.print(*b++);
  }
  Serial.print(F(" (-0)"));
  Serial.println();

  delay(10);
}

void printVersion(void)
{
  Serial.print(firmware_version[0]);
  Serial.print(".");
  Serial.print(firmware_version[1]);
  Serial.print(".");
  Serial.println(firmware_version[2]);
}  
