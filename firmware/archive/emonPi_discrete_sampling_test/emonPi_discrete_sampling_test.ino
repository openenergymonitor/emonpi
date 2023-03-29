// emonPi discrete sampling test sketch 

// Tell emonLib this is the emonPi
// - don't read Vcc assume Vcc = 3.3V as is always the case on emonTx V3
// - eliminates bandgap error and need for calibration http://harizanov.com/2013/09/thoughts-on-avr-adc-accuracy/
#define emonTxV3

// Include Emon Library
#include "EmonLib.h" 

EnergyMonitor ct1, ct2;

const byte emonPi_nodeID=5; 
const byte emonpi_LED_pin=9;

unsigned long lastpost = 0;

void setup()
{    
  pinMode(emonpi_LED_pin, OUTPUT);
  digitalWrite(emonpi_LED_pin, HIGH);
  Serial.begin(9600);
  Serial.println("ATmega328 Startup");
  Serial.println("emonPi discrete sampling test sketch ");
  
  // Calibration, phase_shift
  ct1.voltage(0, 268.97, 1.7);        //ADC pin number, calibration, phase shift           
  ct2.voltage(0, 268.97, 1.7);

  
  // CT Current calibration 
  // (2000 turns / 22 Ohm burden resistor = 90.909)
  ct1.current(1, 60.606); 
  ct2.current(2, 60.606 );
  
  lastpost = 0;
  digitalWrite(emonpi_LED_pin, LOW);
}

void loop()
{ 
  // A simple timer to fix the post rate to once every 10 seconds
  // Please dont post faster than once every 5 seconds to emoncms.org
  // Host your own local installation of emoncms for higher resolutions
  if ((millis()-lastpost)>=10000)
  {
    digitalWrite(emonpi_LED_pin, HIGH);
    lastpost = millis();
    
    // .calcVI: Calculate all. No.of half wavelengths (crossings), time-out 
    ct1.calcVI(20,2000);
    ct2.calcVI(20,2000);

    
    // Print to serial - following oem gateway (now called emonHub) serial listner format 
    Serial.print(emonPi_nodeID); Serial.print(' ');
    Serial.print(ct1.realPower); Serial.print(' '); 
    Serial.print(ct2.realPower); Serial.print(' '); 
    Serial.println(ct1.Vrms);
    
    // Note: the following measurements are also available:
    // - ct1.apparentPower
    // - ct1.Vrms
    // - ct1.Irms
    digitalWrite(emonpi_LED_pin, LOW);
  }
}
