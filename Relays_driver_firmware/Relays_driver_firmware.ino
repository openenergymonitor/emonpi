#define emonTxV3                                                                          // Tell emonLib this is the emonTx V3 - don't read Vcc assume Vcc = 3.3V as is always the case on emonTx V3 eliminates bandgap error and need for calibration http://harizanov.com/2013/09/thoughts-on-avr-adc-accuracy/
#define RF69_COMPAT 1                                                              // Set to 1 if using RFM69CW or 0 is using RFM12B
#include <JeeLib.h>                                                                      //https://github.com/jcw/jeelib - Tested with JeeLib 3/11/14
ISR(WDT_vect) { Sleepy::watchdogEvent(); }                            // Attached JeeLib sleep function to Atmega328 watchdog -enables MCU to be put into sleep mode inbetween readings to reduce power consumption 
#include "EmonLib.h" 

#include "EmonLib.h"  
typedef struct { 
  int relay1_status;
  int relay2_status;
  int relay3_status;
  int relay4_status;
} Playload;     // create structure - a neat way of packaging data for RF comms

Playload load_monitor ;

//-----------------------RFM12B / RFM69CW SETTINGS----------------------------------------------------------------------------------------------------
byte RF_freq=RF12_433MHZ;                                        // Frequency of RF69CW module can be RF12_433MHZ, RF12_868MHZ or RF12_915MHZ. You should use the one matching the module you have.
byte nodeID=8;                                                 // emonpi node ID
uint32_t networkGroup = 210;
boolean RF_STATUS =1;
static int intRFtime = 600000;
// RF Global Variables 
static byte stack[RF12_MAXDATA+4], top, sendLen, dest;           // RF variables 
static char cmd;
static word value;                                               // Used to store serial input

unsigned long last_relay_driver_check = 0;        
const long check_relays_interval = 2000; 


const int relay_pins [] = {9,8,7,6};   // pin number of relays; 
const byte number_of_relays=4;
word relays_status [] = {number_of_relays};

void setup()
{
  Serial.begin(38400);
    for(int index=0; index<number_of_relays; index++)
   {
       pinMode(relay_pins [index], OUTPUT); // Set the mode to OUTPUT
       digitalWrite(relay_pins [index], HIGH);
   }
    //Serial.print("emonTx V3.4 Discrete Sampling V"); Serial.print(firmware_version*0.1);
  #if (RF69_COMPAT)
    Serial.println(" RFM69CW found ");
  #else
    Serial.println(" RFM12B");
  #endif
  rf12_initialize(nodeID, RF_freq, networkGroup);                          // initialize RFM12B/rfm69CW

  
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
    Serial.println(" ");  

  }  
}

void loop()
{ 
   RF_Rx_Handle(); 
 //---------------------------------------------send relay status---------------------------------------------------
   unsigned long current_time = millis();    // without millis function the module is not able to receive and send, it receives only
  if(current_time - last_relay_driver_check >= check_relays_interval)
  {
      Serial.print("Sending relay driver status:" );  
      
      last_relay_driver_check = current_time;    
      load_monitor.relay1_status=relays_status[0];
      load_monitor.relay2_status=relays_status[2];
      load_monitor.relay3_status=relays_status[4];
      load_monitor.relay4_status=relays_status[6];
      rf12_sleep(RF12_WAKEUP);                                   
      rf12_sendNow(0, &load_monitor, sizeof load_monitor);                           //send load_monitor data via RFM12B using new rf12_sendNow wrapper
      rf12_sendWait(.5);
  }    
}


boolean RF_Rx_Handle()
{
  //------------------------------------------------------------------ 
  if (rf12_recvDone()){            //if RF Packet is received 
      byte n = rf12_len;
      if (rf12_crc == 0)              //Check packet is good
      {
        Serial.print("Ok");
        Serial.print(" ");              //Print RF packet to serial in struct format
        Serial.print(rf12_hdr & 0x1F);        // Extract and print node ID
        if (rf12_hdr == 5)
       {
            Serial.print(" "); 
            Serial.print(' ');
               
        for (byte i = 0; i < n; ++i) { 
          relays_status[i] = (word)rf12_data[i];        
           Serial.print((word)rf12_data[i]);
            Serial.print(' ');
            Serial.print(' ');
            
            if ( (word)rf12_data[ i ] == 1)
            {
              digitalWrite(relay_pins [i],LOW);
            }
            else
            {
              digitalWrite(relay_pins [i], HIGH);
            }
            

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
  }

      return(1);
     } 
         
            else
            {
              Serial.println(" Data are not comming from Base");
            } 
                    
      }
      else
      return(0);        
  } //end recDone
  





