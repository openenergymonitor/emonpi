 
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

  Serial.begin(BAUD_RATE);
  Serial.print("emonPi Discrete Sampling V"); Serial.println(firmware_version);
  Serial.println("OpenEnergyMonitor.org");
  Serial.println("POST.....wait 10s");
  
  
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
  double vrms = calc_rms(0,1780) * 0.87;      //double vrms = calc_rms(0,1780) * (Vcal * (3.3/1024) );
  if (vrms>90) ACAC = 1; else ACAC=0;
 
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
 //-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 
      if (debug==1)
      {
        Serial.print("CT 1 Calibration: "); Serial.println(Ical1);
        Serial.print("CT 2 Calibration: "); Serial.println(Ical2);
        delay(1000);
    
        Serial.print("RMS Voltage on AC-AC Adapter input is: ~");
        Serial.print(vrms,0); Serial.println("V");
          
        if (ACAC) 
        {
          Serial.println("AC-AC adapter detected - Real Power measurements enabled");
          if (USA==TRUE) Serial.println("USA mode activated"); 
          Serial.print("Vcal: "); Serial.println(Vcal);
          Serial.print("Phase Shift: "); Serial.println(phase_shift);
        }
         else 
         {
           Serial.println("AC-AC adapter NOT detected - Apparent Power measurements enabled");
           Serial.print("Assuming VRMS to be "); Serial.print(Vrms); Serial.println("V");
         }  
    
        if (CT_count==0) Serial.println("NO CT's detected, sampling from CT1 by default");
        else   
        {
          if (CT1) Serial.println("CT 1 detected");
          if (CT2) Serial.println("CT 2 detected");
        }
        if (DS18B20_STATUS==1) {Serial.print("Detected "); Serial.print(numSensors); Serial.println(" DS18B20..using this for temperature reading");}
          else Serial.println("Unable to detect DS18B20 temperature sensor");
       
        #if (RF69_COMPAT)
           Serial.println("RFM69CW Initiated: ");
        #else
          Serial.println("RFM12B Initiated: ");
        #endif
        
        
        Serial.print("Node: "); Serial.print(nodeID); 
        Serial.print(" Freq: "); 
        if (RF_freq == RF12_433MHZ) Serial.print("433Mhz");
        if (RF_freq == RF12_868MHZ) Serial.print("868Mhz");
        if (RF_freq == RF12_915MHZ) Serial.print("915Mhz"); 
        Serial.print(" Network: "); Serial.println(networkGroup);
       delay(500);  
      }
      else 
        Serial.end();
  }
  
