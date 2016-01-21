// This sketch provides continuous monitoring of real power on four channels, 
// which are split across two phases.  The interrupt-based kernel was kindly 
// provided by Jorg Becker.
//
//      Robin Emley (calypso_rae on Open Energy Monitor Forum)
//      October 2013


#define RF69_COMPAT 1 // define this to use the RF69 driver i.s.o. RF12
#include <JeeLib.h>     // RFu_JeeLib is available at from: http://github.com/openenergymonitor/RFu_jeelib



#include <TimerOne.h>
#define ADC_TIMER_PERIOD 125 // uS
#define MAX_INTERVAL_BETWEEN_CONSECUTIVE_PEAKS 12 // mS

// In this sketch, the ADC is free-running with a cycle time of ~104uS.

//  WORKLOAD_CHECK is available for determining how much spare processing time there 
//  is.  To activate this mode, the #define line below should be included: 
//#define WORKLOAD_CHECK  

// ----------------- RF setup  ---------------------

unsigned long sendinterval = 10000; // milliseconds

#define freq RF12_433MHZ // Use the freq to match the module you have.

const int nodeID = 10;  // emonTx RFM12B node ID
const int networkGroup = 210;  // emonTx RFM12B wireless network group - needs to be same as emonBase and emonGLCD 
const int UNO = 1;  // Set to 0 if you're not using the UNO bootloader (i.e using Duemilanove) 
                                               // - All Atmega's shipped from OpenEnergyMonitor come with Arduino Uno bootloader
 typedef struct { 
   unsigned long msgNumber; 
   int realPower_CT1;
   int realPower_CT2;
   long wh_CT1;
   long wh_CT2;
} Tx_struct;    // revised data for RF comms
Tx_struct tx_data;

// -----------------------------------------------------

enum polarities {NEGATIVE, POSITIVE};
enum LEDstates {LED_OFF, LED_ON};   
enum voltageZones {NEGATIVE_ZONE, MIDDLE_ZONE, POSITIVE_ZONE};

// ----------- Pinout assignments  -----------
//
// digital pins:
// dig pin 0 is for Serial Rx
// dig pin 1 is for Serial Tx
// dig pin 2 is for the RFM12B module (IRQ) 
// dig pin 10 is for the RFM12B module (SEL) 
// dig pin 11 is for the RFM12B module (SDI) 
// dig pin 12 is for the RFM12B module (SDO) 
// dig pin 13 is for the RFM12B module (CLK) 

const byte LEDpin = 9;

// analogue input pins 
const byte voltageSensor = 0;      
const byte currentSensor_CT1 = 1;  
const byte currentSensor_CT2 = 2;      
 


// --------------  general global variables -----------------
//
// Some of these variables are used in multiple blocks so cannot be static.
// For integer maths, many variables need to be 'long'
//
boolean beyondStartUpPhase = false;    // start-up delay, allows things to settle
const byte startUpPeriod = 3;      // in seconds, to allow LP filter to settle
const int DCoffset_I = 512;        // nominal mid-point value of ADC @ x1 scale

int phaseCal_int_CT1;                  // to avoid the need for floating-point maths
int phaseCal_int_CT2;                  // to avoid the need for floating-point maths
long DCoffset_V_long;              // <--- for LPF
long DCoffsetV_min;               // <--- for LPF
long DCoffsetV_max;               // <--- for LPF

// for interaction between the main processor and the ISR 
volatile boolean dataReady = false;
int sample_V;
int sample_CT1;
int sample_CT2;



// Calibration values
//-------------------
// Two calibration values are used in this sketch: powerCal, and phaseCal. 
// With most hardware, the default values are likely to work fine without 
// need for change.  A compact explanation of each of these values now follows:

// When calculating real power, which is what this code does, the individual 
// conversion rates for voltage and current are not of importance.  It is 
// only the conversion rate for POWER which is important.  This is the 
// product of the individual conversion rates for voltage and current.  It 
// therefore has the units of ADC-steps squared per Watt.  Most systems will
// have a power conversion rate of around 20 (ADC-steps squared per Watt).
// 
// powerCal is the RECIPR0CAL of the power conversion rate.  A good value 
// to start with is therefore 1/20 = 0.05 (Watts per ADC-step squared)
//

// Voltage calibration constant:

// AC-AC Voltage adapter is designed to step down the voltage from 230V to 9V
// but the AC Voltage adapter is running open circuit and so output voltage is
// likely to be 20% higher than 9V (9 x 1.2) = 10.8V. 
// Open circuit step down = 230 / 10.8 = 21.3

// The output voltage is then steped down further with the voltage divider which has 
// values Rb = 10k, Rt = 120k (which will reduce the voltage by 13 times.

// The combined step down is therefore 21.3 x 13 = 276.9 which is the 
// theoretical calibration constant entered below.

// Current calibration constant:
// Current calibration constant = 2000 / 22 Ohms burden resistor (The CT sensor has a ratio of 2000:1)

const float powerCal_CT1 = (276.9*(3.3/1023))*(90.9*(3.3/1023)); // <---- powerCal value
const float powerCal_CT2 = (276.9*(3.3/1023))*(90.9*(3.3/1023)); // <---- powerCal value

//const float powerCal_CT1 = 0.0416;  // <---- powerCal value  
//const float powerCal_CT2 = 0.0416;  // <---- powerCal value   
  
                        
// phaseCal is used to alter the phase of the voltage waveform relative to the
// current waveform.  The algorithm interpolates between the most recent pair
// of voltage samples according to the value of phaseCal. 
//
//    With phaseCal = 1, the most recent sample is used.  
//    With phaseCal = 0, the previous sample is used
//    With phaseCal = 0.5, the mid-point (average) value in used
//
// NB. Any tool which determines the optimal value of phaseCal must have a similar 
// scheme for taking sample values as does this sketch!
// http://openenergymonitor.org/emon/node/3792#comment-18683
const float  phaseCal_CT1 = 0.22;
const float  phaseCal_CT2 = 0.41;


int joules_CT1 = 0;
int joules_CT2 = 0;


// for voltage failure detection logic
int voltageThresholdOffset = 250; // in ADC steps from mid-point
long voltageThesholdUpper_long;  // determined once in setup()
long voltageThesholdLower_long;  // determined once in setup()
unsigned long timeOfLastPeakEntry; // would be better as a static in the processing function
enum voltageZones voltageZoneOfLastSampleV; // would be better as a static in the processing function
enum voltageZones lastPeakZoneVisited; // would be better as a static in the processing function

unsigned long lastsecond = millis();
boolean laststate = true;
unsigned long lastsendtime = 0;


void setup()
{  
  rf12_initialize(nodeID, freq, networkGroup);             // initialize RF
  rf12_sleep(RF12_SLEEP);
  
  Serial.begin(9600);                                      // initialize Serial interface
  Serial.println();
  Serial.println();
  Serial.println();
  Serial.println("----------------------------------");
  Serial.println("Sketch ID:  TwoPhaseFourChannel_RPmonitor_1.ino");
  
  pinMode(LEDpin, OUTPUT); 
  digitalWrite(LEDpin, LED_OFF); 

       
  // When using integer maths, calibration values that have supplied in floating point 
  // form need to be rescaled.  
  //
  phaseCal_int_CT1 = phaseCal_CT1 * 256; // for integer maths
  phaseCal_int_CT2 = phaseCal_CT2 * 256; // for integer maths
    
  // Define operating limits for the LP filters which identify DC offset in the voltage 
  // sample streams.  By limiting the output range, these filters always should start up 
  // correctly.
  DCoffset_V_long = 512L * 256; // nominal mid-point value of ADC @ x256 scale
  DCoffsetV_min = (long)(512L - 100) * 256; // mid-point of ADC minus a working margin
  DCoffsetV_max = (long)(512L + 100) * 256; // mid-point of ADC minus a working margin
  
  Serial.println ("ADC mode:       free-running");
  
  // Set up the ADC to be free-running 
  ADCSRA  = (1<<ADPS0)+(1<<ADPS1)+(1<<ADPS2);  // Set the ADC's clock to system clock / 128
  ADCSRA |= (1 << ADEN);                 // Enable the ADC 
  
  ADCSRA |= (1<<ADATE);  // set the Auto Trigger Enable bit in the ADCSRA register.  Because 
                         // bits ADTS0-2 have not been set (i.e. they are all zero), the 
                         // ADC's trigger source is set to "free running mode".
                         
  ADCSRA |=(1<<ADIE);    // set the ADC interrupt enable bit. When this bit is written 
                         // to one and the I-bit in SREG is set, the 
                         // ADC Conversion Complete Interrupt is activated. 

  ADCSRA |= (1<<ADSC);   // start ADC manually first time 
  sei();                 // Enable Global Interrupts  

     
  char flag = 0;
  Serial.print ( "Extra Features: ");  
#ifdef WORKLOAD_CHECK  
  Serial.print ( "WORKLOAD_CHECK ");
  flag++;
#endif
  if (flag == 0) {
    Serial.print ("none"); }
  Serial.println ();
        
  Serial.print ( "powerCal_CT1 =      "); Serial.println (powerCal_CT1,4);
  Serial.print ( "phaseCal_CT1 =      "); Serial.println (phaseCal_CT1);
  Serial.print ( "powerCal_CT2 =      "); Serial.println (powerCal_CT2,4);
  Serial.print ( "phaseCal_CT2 =      "); Serial.println (phaseCal_CT2);

  
  Serial.println ("----");    

#ifdef WORKLOAD_CHECK
   Serial.println ("WELCOME TO WORKLOAD_CHECK ");
  
//   <<- start of commented out section, to save on RAM space!
/*   
   Serial.println ("  This mode of operation allows the spare processing capacity of the system");
   Serial.println ("to be analysed.  Additional delay is gradually increased until all spare time");
   Serial.println ("has been used up.  This value (in uS) is noted and the process is repeated.  ");
   Serial.println ("The delay setting is increased by 1uS at a time, and each value of delay is ");
   Serial.println ("checked several times before the delay is increased. "); 
 */ 
//  <<- end of commented out section, to save on RAM space!

   Serial.println ("  The displayed value is the amount of spare time, per pair of V & I samples, ");
   Serial.println ("that is available for doing additional processing.");
   Serial.println ();
 #endif
 
  voltageThesholdUpper_long = (long)voltageThresholdOffset << 8;
  voltageThesholdLower_long = -1 * voltageThesholdUpper_long;
  Serial.println("voltage thresholds long:");
  Serial.print("  upper: ");
  Serial.println(voltageThesholdUpper_long);
  Serial.print("  lower: ");
  Serial.println(voltageThesholdLower_long);


  tx_data.msgNumber = 0;
  tx_data.wh_CT1 = 0;
  tx_data.wh_CT2 = 0;

  
}

// An Interrupt Service Routine is now defined in which the ADC is instructed to perform 
// a conversion of the voltage signal and each of the signals for current.  A "data ready" 
// flag is set after each voltage conversion has been completed, it being the last one in
// the sequence.  
//   Samples for current are taken first because the phase of the waveform for current is 
// generally slightly advanced relative to the waveform for voltage.  The data ready flag 
// is cleared within loop().

// This Interrupt Service Routine is for use when the ADC is in the free-running mode.
// It is executed whenever an ADC conversion has finished, approx every 104 us.  In 
// free-running mode, the ADC has already started its next conversion by the time that
// the ISR is executed.  The ISR therefore needs to "look ahead". 
//   At the end of conversion Type N, conversion Type N+1 will start automatically.  The ISR 
// which runs at this point therefore needs to capture the results of conversion Type N , 
// and set up the conditions for conversion Type N+2, and so on.  
// 
ISR(ADC_vect)  
{                                         
  static unsigned char sample_index = 0;
  
  switch(sample_index)
  {
    case 0:
      sample_V = ADC; 
      ADMUX = 0x40 + currentSensor_CT2; // set up the next-but-one conversion
      sample_index++; // advance the control flag             
      dataReady = true; 
      break;
    case 1:
      sample_CT1 = ADC; 
      sample_index++; // advance the control flag                
      break;
    case 2:

      sample_index = 0;                 // to prevent lockup (should never get here)      
  }  
}


// When using interrupt-based logic, the main processor waits in loop() until the 
// dataReady flag has been set by the ADC.  Once this flag has been set, the main
// processor clears the flag and proceeds with all the processing for one pair of 
// V & I samples.  It then returns to loop() to wait for the next pair to become 
// available.
//   If the next pair of samples become available before the processing of the 
// previous pair has been completed, data could be lost.  This situation can be 
// avoided by prior use of the WORKLOAD_CHECK mode.  Using this facility, the amount
// of spare processing capacity per loop can be determined.  
//
void loop()             
{ 
#ifdef WORKLOAD_CHECK
  static int del = 0; // delay, as passed to delayMicroseconds()
  static int res = 0; // result, to be displayed at the next opportunity
  static byte count = 0; // to allow multiple runs per setting
  static byte displayFlag = 0; // to determine when printing may occur
#endif
  
  if (dataReady)   // flag is set after every pair of ADC conversions
  {
    dataReady = false; // reset the flag
    allGeneralProcessing(); // executed once for each pair of V&I samples
    
#ifdef WORKLOAD_CHECK 
    delayMicroseconds(del); // <--- to assess how much spare time there is
    if (dataReady)       // if data is ready again, delay was too long
    { 
      res = del;             // note the exact value
      del = 1;               // and start again with 1us delay   
      count = 0;
      displayFlag = 0;   
    }
    else
    {
      count++;          // to give several runs with the same value
      if (count > 50)
      {
        count = 0;
        del++;          //  increase delay by 1uS
      } 
    }
#endif  

  }  // <-- this closing brace needs to be outside the WORKLOAD_CHECK blocks! 
  
#ifdef WORKLOAD_CHECK 
  switch (displayFlag) 
  {
    case 0: // the result is available now, but don't display until the next loop
      displayFlag++;
      break;
    case 1: // with minimum delay, it's OK to print now
      Serial.print(res);
      displayFlag++;
      break;
    case 2: // with minimum delay, it's OK to print now
      Serial.println("uS");
      displayFlag++;
      break;
    default:; // for most of the time, displayFlag is 3           
  }
#endif
  
  if ((millis()-lastsendtime)>sendinterval)
  {
    lastsendtime = millis();
    send_rf_data();  //  dispatch the RF message
    Serial.println(tx_data.msgNumber++);  
  }
  
} // end of loop()


// This routine is called to process each pair of V & I samples.  Note that when using 
// interrupt-based code, it is not necessary to delay the processing of each pair of 
// samples as was done in Mk2a builds.  This is because there is no longer a strict 
// alignment between the obtaining of each sample by the ADC and the processing that can 
// be done by the main processor while the ADC conversion is in progress.  
//   When interrupts are used, the main processor and the ADC work autonomously, their
// operation being only linked via the dataReady flag.  As soon as data is made available
// by the ADC, the main processor can start to work on it immediately.  
//
void allGeneralProcessing()
{
  static long sumP_CT1;                         
  static long sumP_CT2;                              
;                            
  static enum polarities polarityOfLastSampleV;  // for zero-crossing detection
  static long cumV_deltasThisCycle_long;    // for the LPF which determines DC offset (voltage)
  static long lastSampleV_minusDC_long;     //    for the phaseCal algorithm
  static long msgNumber = 0;

  static int sequenceCount = 0;
  static int samplesDuringThisWindow_CT1;            
  static int samplesDuringThisWindow_CT2;            
           

  // remove DC offset from the raw voltage sample by subtracting the accurate value 
  // as determined by a LP filter.
  long sampleV_minusDC_long = ((long)sample_V<<8) - DCoffset_V_long;

  // for AC failure detection 
  enum voltageZones voltageZoneNow; 
  boolean nextPeakDetected = false;
  unsigned long timeNow = millis();
  

  // determine polarity, to aid the logical flow
  enum polarities polarityNow;   
  if(sampleV_minusDC_long > 0) { 
    polarityNow = POSITIVE; }
  else { 
    polarityNow = NEGATIVE; }

  if (polarityNow == POSITIVE) 
  {                           
    if (beyondStartUpPhase)
    {  
      if (polarityOfLastSampleV != POSITIVE)
      {
        // This is the start of a new +ve half cycle (just after the zero-crossing point)
        sequenceCount++;
        static char separatorString[5] = ", ";
        long realPower_long;
        
        switch(sequenceCount)
        {
          // 100 cycles = 2 seconds
          case 20: 
            realPower_long = sumP_CT1 / samplesDuringThisWindow_CT1; 
            tx_data.realPower_CT1 = realPower_long * powerCal_CT1;
            
            joules_CT1 += tx_data.realPower_CT1 * 2;  // Joules elapsed in 100 cycles @ 50Hz is power J.s x 2 seconds
            
            tx_data.wh_CT1 += joules_CT1 / 3600;
            joules_CT1 = joules_CT1 % 3600;
            
            Serial.print("samples per mains cycle = " );           
            Serial.print(samplesDuringThisWindow_CT1 /100.0);
            Serial.print(" ");
            Serial.println();      
            sumP_CT1 = 0;
            samplesDuringThisWindow_CT1 = 0;
            Serial.print(tx_data.realPower_CT1);
            Serial.print(':');
            Serial.print(tx_data.wh_CT1);
            break;
          case 40: 
            realPower_long = sumP_CT2 / samplesDuringThisWindow_CT2; 
            tx_data.realPower_CT2 = realPower_long * powerCal_CT2;
            
            joules_CT2 += tx_data.realPower_CT2 * 2;  // Joules elapsed in 100 cycles @ 50Hz is power J.s x 2 seconds
            
            tx_data.wh_CT2 += joules_CT2 / 3600;
            joules_CT2 = joules_CT2 % 3600;
            
            sumP_CT2 = 0;
            samplesDuringThisWindow_CT2 = 0;
            Serial.print(separatorString);
            Serial.print(tx_data.realPower_CT2); 
            Serial.print(':');
            Serial.print(tx_data.wh_CT2);          
            break;
          
            
          
          case 100: 
            tx_data.msgNumber = msgNumber++;
  
            sequenceCount = 0;
            Serial.print(separatorString);
                     
            break;
          default: 
            if (sequenceCount > 100) {
              // should never get here
              sequenceCount = 0; }         
        }
        
        // Calculate the average real power during the measurement period.
        //
        // sumP contains the sum of many individual calculations of instantaneous power.  In  
        // order to obtain the average power during the relevant period, sumP must be divided 
        // by the number of samples that have contributed to its value.
        //
        
        // The next stage is to apply a calibration factor so that real power can be expressed 
        // in Watts.  That's fine for floating point maths, but it's not such
        // a good idea when integer maths is being used.  To keep the numbers large, and also 
        // to save time, calibration of power is omitted at this stage.  realPower_long is 
        // therefore (1/powerCal) times larger than the actual power in Watts.
        //
        long realPower_long_CT1 = sumP_CT1 / samplesDuringThisWindow_CT1; 
        long realPower_long_CT2 = sumP_CT2 / samplesDuringThisWindow_CT2; 

   
 
      } // end of processing that is specific to the first Vsample in each +ve half cycle   
    }
    else
    {  
      // wait until the DC-blocking filters have had time to settle
      if(millis() > startUpPeriod * 1000) 
      {
        beyondStartUpPhase = true;
        Serial.println ("Go!");
      }
    }
  } // end of processing that is specific to samples where the voltage is positive
  
  else // the polarity of this sample is negative
  {     
    if (polarityOfLastSampleV != NEGATIVE)
    {
      // This is the start of a new -ve half cycle (just after the zero-crossing point)
      //
      // This is a convenient point to update the twin Low Pass Filters for DC-offset removal,
      // one on each voltage channel.  This needs to be done right from the start.
      long previousOffset; 
      
      previousOffset = DCoffset_V_long; // for voltage source V
      DCoffset_V_long = previousOffset + (cumV_deltasThisCycle_long>>6); // faster than * 0.01
      cumV_deltasThisCycle_long = 0;
      
      // To ensure that each of these LP filters will always start up correctly when 240V AC is 
      // available, its output value needs to be prevented from drifting beyond the likely range 
      // of the voltage signal.  This avoids the need to include a HPF as is often used for 
      // sketches of this type.
      //
      if (DCoffset_V_long < DCoffsetV_min) {  // for voltage source V
        DCoffset_V_long = DCoffsetV_min; }
      else  
      if (DCoffset_V_long > DCoffsetV_max) {
        DCoffset_V_long = DCoffsetV_max; }
                
    } // end of processing that is specific to the first Vsample in each -ve half cycle
  } // end of processing that is specific to samples where the voltage is positive
  
  // Processing for EVERY pair of samples. Most of this code is not used during the 
  // start-up period, but it does no harm to leave it in place.  Accumulated values 
  // are cleared when beyondStartUpPhase is set to true.
  //
  // remove most of the DC offset from the current sample (the precise value does not matter)
  long sampleIminusDC_long_CT1 = ((long)(sample_CT1 - DCoffset_I))<<8;
  long sampleIminusDC_long_CT2 = ((long)(sample_CT2 - DCoffset_I))<<8;

  
  // phase-shift the voltage waveform so that it aligns with the current when a 
  // resistive load is used
  long  phaseShiftedSampleV_minusDC_long_CT1 = lastSampleV_minusDC_long
         + (((sampleV_minusDC_long - lastSampleV_minusDC_long)*phaseCal_int_CT1)>>8);  
  long  phaseShiftedSampleV_minusDC_long_CT2 = lastSampleV_minusDC_long
         + (((sampleV_minusDC_long - lastSampleV_minusDC_long)*phaseCal_int_CT2)>>8);

  
  // calculate the "real power" in this sample pair and add to the accumulated sum
  long filtV_div4_CT1 = phaseShiftedSampleV_minusDC_long_CT1>>2;  // reduce to 16-bits (now x64, or 2^6)
  long filtI_div4_CT1 = sampleIminusDC_long_CT1>>2; // reduce to 16-bits (now x64, or 2^6)
  long instP_CT1 = filtV_div4_CT1 * filtI_div4_CT1;  // 32-bits (now x4096, or 2^12)
  instP_CT1 = instP_CT1>>12;     // scaling is now x1, as for Mk2 (V_ADC x I_ADC)  

  if (sample_CT1==0) instP_CT1 = 0;  
  sumP_CT1 +=instP_CT1; // cumulative power, scaling as for Mk2 (V_ADC x I_ADC)
  
  long filtV_div4_CT2 = phaseShiftedSampleV_minusDC_long_CT2>>2;  // reduce to 16-bits (now x64, or 2^6)
  long filtI_div4_CT2 = sampleIminusDC_long_CT2>>2; // reduce to 16-bits (now x64, or 2^6)
  long instP_CT2 = filtV_div4_CT2 * filtI_div4_CT2;  // 32-bits (now x4096, or 2^12)
  instP_CT2 = instP_CT2>>12;     // scaling is now x1, as for Mk2 (V_ADC x I_ADC)

  if (sample_CT2==0) instP_CT2 = 0;  
  sumP_CT2 +=instP_CT2; // cumulative power, scaling as for Mk2 (V_ADC x I_ADC)
  
  
  samplesDuringThisWindow_CT1++;
  samplesDuringThisWindow_CT2++;

  
  // store items for use during next loop
  cumV_deltasThisCycle_long += sampleV_minusDC_long; // for use with LP filter
  lastSampleV_minusDC_long = sampleV_minusDC_long;  // required for phaseCal algorithm
  
  polarityOfLastSampleV = polarityNow;  // for identification of half cycle boundaries
  voltageZoneOfLastSampleV = voltageZoneNow; // for voltage failure detection
}
// end of allGeneralProcessing()


void send_rf_data()
{
  rf12_sleep(RF12_WAKEUP);
  // if ready to send + exit route if it gets stuck 
  int i = 0; 
  while (!rf12_canSend() && i<10)
  { 
    rf12_recvDone(); 
    i++;
  }
  rf12_sendStart(0, &tx_data, sizeof tx_data);
  rf12_sendWait(2);
  rf12_sleep(RF12_SLEEP);
}

int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}




