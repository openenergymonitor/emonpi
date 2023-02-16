/*


  Header for emonPi Continuous Monitoring - radio using JeeLib RFM69 "Native" format


   ------------------------------------------
  Part of the openenergymonitor.org project

  Authors: Glyn Hudson, Trystan Lea & Robert Wall
  Builds upon JCW JeeLabs RF69 Driver and Arduino

  Licence: GNU GPL V3

  V1.0.0   10/7/2021 Derived from emonLibCM examples and original emonPi sketch, that being derived from
            https://github.com/openenergymonitor/emonpi/blob/master/Atmega328/emonPi_RFM69CW_RF12Demo_DiscreteSampling
            and emonLibCM example sketches, with config input based on emonTx V3 sketches.


  Config functions for emonPiCM

  EEPROM layout

  Byte
  0       RF Band
  1       Group
  2-5     vCal
  6-9     line frequency
  10-13   i1Cal
  14-17   i1Lead
  18-21   i2Cal
  22-25   i2Lead
  26-29   Datalogging period
  30      Pulses enabled
  31-34   Pulse min period
  35      Temperatures enabled
  36-83   Temperature Addresses (6×8)
  84      showCurrents print enabled
  85      rfOn
*/


#include <Arduino.h>
#include <avr/pgmspace.h>
#include <EEPROM.h>

// Available Serial Commands
const PROGMEM char helpText1[] =
  "|\n"
  "|Available commands:\n"
  "| l\t\t- list config (terse)\n"
  "| L\t\t- list config (verbose)\n"
  "| r\t\t- restore defaults & restart\n"
  "| s\t\t- save to EEPROM\n"
  "| v\t\t- show version\n"
  "| V<n>\t\t- verbose mode, 1 > ON, 0 > OFF\n"
  "| b<n>\t\t- set r.f. band n = 4 > 433MHz, 8 > 868MHz, 9 > 915MHz (may require hardware change)\n"
  "| p<nn>\t\t- set r.f. power. nn (0 - 31) = -18 dBm to +13 dBm. Default: 25 (+7 dBm)\n"
  "| g<nnn>\t- set Group (OEM default = 210)\n"
  "| n<nn>\t\t- set node ID (1..60)\n"
  "| c<n>\t\t- enable output for calibration. n = 0|1 \n"
  "| d<xx.x>\t- datalogging period (s)\n"
  "| k<x> <yy.y> <zz.z>\n"
  "|\t\t- Calibrate input:\n"
  "|\t\t\tx = 0 > voltage, 1 > ct1, 2 > ct2, etc\n"
  "|\t\t\tyy.y = voltage/current calibration\n"
  "|\t\t\tzz.z = phase calibration. (ct only)\n"
  "|\t\t\te.g. k0 256.8\n"
  "|\t\t\tk1 90.9 2.00\n"
  "| f<xx>\t\t- frequency (50 or 60)\n"
  "| a<xx.x>\t- assumed voltage if no a.c. present\n"
  "| m<x> <yy>\t- meter pulse counting:\n"
  "|\t\t\tx = 0 > all OFF, x = 1 > count 1 ON, x = 2 > count 2 ON, <yy> = pulse minimum period (ms) (y is not needed when x = 0)\n"
  "| t0 <y>\t- temperature:\n"
  "|\t\t\ty = 0 > OFF, y = 1 > ON, Y = 2 > Search\n"
  "| t<x> <yy> <yy> <yy> <yy> <yy> <yy> <yy> <yy>\n"
  "|\t\t\tchange sensor's address or position:\n"
  "|\t\t\tx = the sensor position in the list (1-based)\n"
  "|\t\t\tyy = 8 hex bytes - the sensor's address\n"
  "|\t\t\te.g.  28 81 43 31 07 00 00 D9\n"
  "|\t\t\tN.B. Sensors CANNOT be added beyond the array size.\n"
  "| T<ccc>\\n\t- transmit a string.\n"
  "| w<n>\t\t- n = 0 > radio OFF, n = 1 for Receive ON, n = 2 for Transmit ON, n = 3 for both ON\n"
  "| z\t\t- set the energy values and pulse count(s) to zero\n"
  "| ?\t\t- show this again\n|"
  ;


extern DeviceAddress *temperatureSensors;

#define SERIAL_LOCK 2000                                               // Lockout period (ms) after 'old-style' config command

static void load_config(void)
{
  eepromRead(eepromSig, (byte *)&EEProm);
}

static void list_calibration(void)
{

  Serial.println(F("|Settings"));
  Serial.print(F("|Radio: ")); Serial.println(EEProm.rfOn);
  Serial.print(F("|RF Band: "));
  if (EEProm.RF_freq == RFM_433MHZ) Serial.println(F("433MHz"));
  if (EEProm.RF_freq == RFM_868MHZ) Serial.println(F("868MHz"));
  if (EEProm.RF_freq == RFM_915MHZ) Serial.println(F("915MHz"));
  Serial.print(F("|Power: ")); Serial.print(EEProm.rfPower - 18); Serial.println(F(" dBm"));
  Serial.print(F("|Group: ")); Serial.println(EEProm.networkGroup);
  Serial.print(F("|Node ID: ")); Serial.println(EEProm.nodeID);
  Serial.println(F("|Calibration"));
  Serial.print(F("|assumedV: ")); Serial.println(EEProm.assumedVrms);
  Serial.print(F("|AC Detected: ")); Serial.println(EmonLibCM_acPresent() ? "Yes" : "No");
  Serial.print(F("|freq: ")); Serial.println(EEProm.lineFreq);
  Serial.print(F("|vCal: ")); Serial.println(EEProm.vCal);
  Serial.print(F("|i1Cal: ")); Serial.print(EEProm.i1Cal);
  Serial.print(F(", Lead: ")); Serial.println(EEProm.i1Lead);
  Serial.print(F("|i2Cal: ")); Serial.print(EEProm.i2Cal);
  Serial.print(F(", Lead: ")); Serial.println(EEProm.i2Lead);
  Serial.print(F("|datalog: ")); Serial.println(EEProm.period);
  Serial.print(F("|pulses1: ")); Serial.print(EEProm.pulse_enable);
  Serial.print(F(", period: ")); Serial.println(EEProm.pulse_period);
  Serial.print(F("|pulses2: ")); Serial.print(EEProm.pulse2_enable);
  Serial.print(F(", period: ")); Serial.println(EEProm.pulse2_period);
  Serial.print(F("|temp_enable: ")); Serial.print(EEProm.temp_enable);
  Serial.print(F(", sensors found: ")); Serial.println(EmonLibCM_getTemperatureSensorCount());
  if (verbose)
    printTemperatureSensorAddresses(true);
}

static void report_calibration(void)
{
  Serial.print(F("Settings:"));
  Serial.print(F(" r")); Serial.print(EEProm.rfOn);
  Serial.print(F(" b"));
  if (EEProm.RF_freq == RFM_433MHZ) Serial.print(F("4"));
  if (EEProm.RF_freq == RFM_868MHZ) Serial.print(F("8"));
  if (EEProm.RF_freq == RFM_915MHZ) Serial.print(F("9"));
  Serial.print(F(" p")); Serial.print(EEProm.rfPower);
  Serial.print(F(" g")); Serial.print(EEProm.networkGroup);
  Serial.print(F(" i")); Serial.print(EEProm.nodeID);
  Serial.print(F(" a")); Serial.print(EEProm.assumedVrms);
  Serial.print(F(" ac")); Serial.print(EmonLibCM_acPresent());
  Serial.print(F(" f")); Serial.print(EEProm.lineFreq);
  Serial.print(F(" k0 ")); Serial.print(EEProm.vCal); Serial.print(F(" 0.00"));
  Serial.print(F(" k1 ")); Serial.print(EEProm.i1Cal); Serial.print(F(" ")); Serial.print(EEProm.i1Lead);
  Serial.print(F(" k2 ")); Serial.print(EEProm.i2Cal); Serial.print(F(" ")); Serial.print(EEProm.i2Lead);
  Serial.print(F(" d")); Serial.print(EEProm.period);
  Serial.print(F(" m")); Serial.print(EEProm.pulse_enable); Serial.print(F(" ")); Serial.print(EEProm.pulse_period);
  Serial.print(F(" ")); Serial.print(EEProm.pulse2_enable); Serial.print(F(" ")); Serial.print(EEProm.pulse2_period);
  Serial.print(F(" t")); Serial.print(EEProm.temp_enable);  Serial.print(F(" ")); Serial.println(EmonLibCM_getTemperatureSensorCount());
}

static void save_config()
{
  eepromWrite(eepromSig, (byte *)&EEProm, sizeof(EEProm));
  if (verbose)
  {
    eepromPrint(true);
    Serial.println(F("\r\n|Config saved\r\n|"));
  }
}

static void wipe_eeprom(void)
{
  if (verbose)
  {
    Serial.print(F("|Resetting..."));
  }
  eepromHide(eepromSig);
  if (verbose)
  {
    Serial.println(F("|Sketch restarting with default config."));
  }
}

void softReset(void)
{
  asm volatile ("  jmp 0");
}

void getCalibration(void)
{
  /*
     Reads calibration information (if available) from the serial port at runtime.
     Data is expected generally in the format

      [l] [x] [y] [z]

     where:
      [l] = a single letter denoting the variable to adjust
      [x] [y] [z] are values to be set.
      see the user instruction above, the comments below or the separate documentation for details

  */

  if (Serial.available())
  {
    int k1;
    double k2, k3;
    char c = Serial.peek();
    char* msg;

    if (!lockout(c))
      switch (c) {

        case 'a':
          EEProm.assumedVrms = Serial.parseFloat();
          EmonLibCM_setAssumedVrms(EEProm.assumedVrms);
          if (verbose)
          {
            Serial.print(F("|Assumed V: ")); Serial.println(EEProm.assumedVrms);
          }
          break;

        case 'b':  // set band: 4 = 433, 8 = 868, 9 = 915
          EEProm.RF_freq = bandToFreq(Serial.parseInt());
          if (verbose)
          {
            Serial.print(F("|RF Band = "));
            if (EEProm.RF_freq == RFM_433MHZ) Serial.println(F("433MHz"));
            if (EEProm.RF_freq == RFM_868MHZ) Serial.println(F("868MHz"));
            if (EEProm.RF_freq == RFM_915MHZ) Serial.println(F("915MHz"));
          }
          rfChanged = true;
          break;

        case 'c':
          /*
            Format expected: c0 | c1
          */
          k1 = Serial.parseInt();
          switch (k1) {
            case 1 : EEProm.showCurrents = true;
              break;
            default: EEProm.showCurrents = false;
          }
          break;

        case 'd':
          /*  Format expected: p[x]

             where:
              [x] = a floating point number for the datalogging period in s
          */
          k2 = Serial.parseFloat();
          if (k2 < 0.1)
            k2 = 0.1;
          EmonLibCM_datalog_period(k2);
          EEProm.period = k2;
          if (verbose)
          {
            Serial.print(F("|datalog period ")); Serial.print(k2); Serial.println(F(" s"));
          }
          break;

        case 'f':
          /*
             Format expected: f50 | f60
          */
          k1 = Serial.parseFloat();
          EEProm.lineFreq = k1;
          if (verbose)
          {
            Serial.print(F("|freq ")); Serial.println(EEProm.lineFreq);
          }
          break;

        case 'g':  // set network group
          EEProm.networkGroup = Serial.parseInt();
          if (verbose)
          {
            Serial.print(F("|Group ")); Serial.println(EEProm.networkGroup);
          }
          rfChanged = true;
          break;

        case 'k':
          /*  Format expected: k[x] [y] [z]

            where:
             [x] = a single numeral: 0 = voltage calibration, 1 = ct1 calibration, 2 = ct2 calibration, etc
             [y] = a floating point number for the voltage/current calibration constant
             [z] = a floating point number for the phase calibration for this c.t. (z is not needed, or ignored if supplied, when x = 0)

            e.g. k0 256.8
                 k1 90.9 1.7

            If power factor is not displayed, it is impossible to calibrate for phase errors,
             and the standard value of phase calibration MUST BE SENT when a current calibration is changed.
          */
          k1 = Serial.parseInt();
          k2 = Serial.parseFloat();
          k3 = Serial.parseFloat();
          while (Serial.available())
            Serial.read();

          // Write the values back as Globals, re-calculate intermediate values.
          switch (k1) {
            case 0 : EmonLibCM_ReCalibrate_VChannel(k2);
              EEProm.vCal = k2;
              break;

            case 1 : EmonLibCM_ReCalibrate_IChannel(1, k2, k3);
              EEProm.i1Cal = k2;
              EEProm.i1Lead = k3;
              break;

            case 2 : EmonLibCM_ReCalibrate_IChannel(2, k2, k3);
              EEProm.i2Cal = k2;
              EEProm.i2Lead = k3;
              break;

            default : ;
          }
          if (verbose)
          {
            Serial.print(F("|k")); Serial.print(k1); Serial.print(F(" ")); Serial.print(k2); Serial.print(F(" ")); Serial.println(k3);
          }
          break;

        case 'L':
          list_calibration(); // print the calibration values (verbose)
          break;

        case 'l':
          report_calibration(); // report calibration values to emonHub (terse)
          break;

        case 'm' :
          /*  Format expected: m[x] [y]

             where:
              [x] = a single numeral: 0 = pulses OFF, 1 = pulses 1 ON, 2 = pulses 2 ON
              [y] = an integer for the pulse min period in ms - ignored when x=0
          */
          k1 = Serial.parseInt();
          k2 = Serial.parseInt();
          while (Serial.available())
            Serial.read();

          switch (k1) {
            case 0 : EmonLibCM_setPulseEnable(0, false);
              EEProm.pulse_enable = false;
              EmonLibCM_setPulseEnable(1, false);
              EEProm.pulse2_enable = false;
              break;

            case 1 : EmonLibCM_setPulseMinPeriod(0, k2);
              EmonLibCM_setPulseEnable(true);
              EEProm.pulse_enable = true;
              EEProm.pulse_period = k2;
              break;

            case 2 : EmonLibCM_setPulseMinPeriod(1, k2);
              EmonLibCM_setPulseEnable(2, true);
              EEProm.pulse2_enable = true;
              EEProm.pulse2_period = k2;
              break;
          }
          if (verbose)
          {
            Serial.print(F("|Pulses: "));
            switch (k1) {
              case 0 : Serial.println(F("off"));
                break;

              case 1 : Serial.print(F("Ch 1: "));
                Serial.print(k2);
                Serial.println(F(" ms"));
                break;

              case 2 : Serial.print(F("Ch 2: "));
                Serial.print(k2);
                Serial.println(F(" ms"));
                break;
            }
          }
          break;

        case 'n':
        case 'i':  //  Set NodeID - range expected: 1 - 60
          EEProm.nodeID = Serial.parseInt();
          EEProm.nodeID = constrain(EEProm.nodeID, 1, 63);
          if (verbose)
          {
            Serial.print(F("|Node ")); Serial.println(EEProm.nodeID);
          }
          rfChanged = true;
          break;

        case 'p': // set RF power level
          EEProm.rfPower = (Serial.parseInt() & 0x1F);
          if (verbose)
          {
            Serial.print(F("|p: ")); Serial.print((EEProm.rfPower & 0x1F) - 18); Serial.println(F(" dBm"));
          }
          rfChanged = true;
          break;

        case 'r':
          wipe_eeprom(); // restore sketch defaults
          softReset();
          break;

        case 's' :
          save_config(); // Save to EEPROM. ATMega328p has 1kB  EEPROM
          break;

        case 't' :
          /*  Format expected: t[x] [y] [y] ...
          */
          set_temperatures();
          break;

        case 'T': // write alpha-numeric string to be transmitted.
          outmsgLength = 0;
          char c = 0;

          while (Serial.peek() ) {
            long txDataByte = Serial.parseInt();
            if (txDataByte > 255 || txDataByte < 0) {
              Serial.println("Tx Data invalid.. each byte must be between 0 & 255");
              Serial.println("Usage:T byte1,byte2,byteN,dest_node_id, Max number of bytes = 60");
              outmsgLength = 0 ;        //make sure invalid data not sent
              break;
            } else if (Serial.peek() == -1 && txDataByte == 0) {
              //done, got all the bytes from Serial
              if (outmsgLength != 0)
                 outmsgLength-- ;      //the actual length of the msg is 1 less as the 1st byte received is not part of the payload
              break ;

            } else {
              Serial.print("read:") ;
              Serial.println(txDataByte) ;
              if (outmsgLength == 0) {
                //first byte should be the destination ID, 0 for broadcast
                //This doesn't become part of the message
                txDestId = txDataByte ;
                outmsgLength++ ;
              } else {
                outmsg[outmsgLength - 1] = txDataByte ; //the outmsg index is always -1 due to the destID not being part of outmsg
                outmsgLength++;
              }
            }
          }
          Serial.print ("Queueing  ") ; Serial.print(outmsgLength); Serial.print (" bytes to send to node ") ; Serial.println (txDestId);
          break;

        case 'v': // print firmware version
          Serial.print(F("|emonPi CM V")); printVersion();
          break;

        case 'V': // Verbose mode
          /*
            Format expected: V0 | V1
          */
          verbose = (bool)Serial.parseInt();
          Serial.print(F("|Verbose mode ")); Serial.println(verbose ? F("on") : F("off"));
          break;

        case 'w':
          /*
            Wireless off = 0, tx = 1, rx = 2, tx+rx  = 3
            Format expected: w0 - w3
          */
          EEProm.rfOn = Serial.parseInt();
          if (verbose)
          {
            Serial.print(F("|Radio "));; Serial.println(EEProm.rfOn);
          }
          break;

        case 'z':
          /*
            Zero all energy values
          */
          zeroEValues();
          EmonLibCM_setWattHour(0, 0);
          EmonLibCM_setWattHour(1, 0);
          EmonLibCM_setPulseCount(0, 0);
          EmonLibCM_setPulseCount(1, 0);
          break;


        case '?':  // show Help text
          showString(helpText1);
          Serial.println();
          break;

        default:
          ;
      }
    // flush the input buffer
    while (Serial.available())
      Serial.read();
  }
}

bool lockout(char c)
{
  static bool locked = false;
  static unsigned long locktime;

  if (c > 47 && c < 58)                                                // lock out old 'Reverse Polish' format: numbers first.
  {
    locked = true;
    locktime = millis();
    while (Serial.available())
      Serial.read();
  }
  else if ((millis() - locktime) > SERIAL_LOCK)
  {
    locked = false;
  }
  return locked;
}

static byte bandToFreq (int band) {

  if (band == 4 || band == 433) {
    return RFM_433MHZ ;
  } else if (band == 8 || band == 868) {
    return RFM_868MHZ ;
  } else if (band == 9 || band == 915) {
    return RFM_915MHZ ;
  } else {
    return 0 ;
  }

}

static void showString (PGM_P s)
{
  for (;;)
  {
    char c = pgm_read_byte(s++);
    if (c == 0)
      break;
    if (c == '\n')
      Serial.print('\r');
    Serial.print(c);
  }
}

void set_temperatures(void)
{
  /*  Format expected: t[x] [y] [y] ...

    where:
     [x] = 0  [y] = single numeral: 0 = temperature measurement OFF, 1 = temperature measurement ON, 2 = search
     [x] = a single numeral > 0: the position of the sensor in the list (1-based)
     [y] = 8 hexadecimal bytes representing the sensor's address
             e.g. t2 28 81 43 31 07 00 00 D9
  */

  DeviceAddress sensorAddress;

  int k1 = Serial.parseInt();

  if (k1 == 0)
  {
    byte k2 = Serial.parseInt();
    // write to EEPROM
    switch (k2) {
      case 0:
      case 1:
        EEProm.temp_enable = k2;
        EmonLibCM_TemperatureEnable(EEProm.temp_enable);
        break;
      case 2:   // search & enable
        temperatureSensors[0][0] = 0x00;
        EEProm.temp_enable = true;
        EmonLibCM_TemperatureEnable(true);
        break;
    }
  }
  else if ((unsigned int)k1 > sizeof(EEProm.allAddresses) / sizeof(DeviceAddress))
    return;
  else
  {
    byte i = 0, a = 0, b;
    Serial.readBytes(&b, 1);    // expect a leading space
    while (Serial.readBytes(&b, 1) && i < 8)
    {
      if (b == ' ' || b == '\r' || b == '\n')
      {
        sensorAddress[i++] = a;
        a = 0;
      }
      else
      {
        a *= 16;
        a += c2h(b);
      }
    }
    // set address
    for (byte i = 0; i < 8; i++)
      EEProm.allAddresses[k1 - 1][i] = sensorAddress[i];
  }
  while (Serial.available())
    Serial.read();
}

byte c2h(byte b)
{
  if (b > 47 && b < 58)
    return b - 48;
  else if (b > 64 && b < 71)
    return b - 55;
  else if (b > 96 && b < 103)
    return b - 87;
  return 0;
}
