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

  Serial.print("CT 1 Calibration: "); Serial.println(Ical1);
  Serial.print("CT 2 Calibration: "); Serial.println(Ical2);
  delay(1000);

  Serial.print("RMS Voltage on AC-AC Adapter input is: ~");
  Serial.print(vrms,0); Serial.println("V");

  if (ACAC) 
  {
    lcd.print("AC Wave Detected");
    Serial.println("AC-AC adapter detected - Real Power measurements enabled");
    if (USA==TRUE) Serial.println("USA mode activated"); 
    Serial.print("Vcal: "); Serial.println(Vcal);
    Serial.print("Phase Shift: "); Serial.println(phase_shift);
  }
  else 
  {
    lcd.print("AC NOT Detected");
   Serial.println("AC-AC adapter NOT detected - Apparent Power measurements enabled");
   Serial.print("Assuming VRMS to be "); Serial.print(Vrms); Serial.println("V");
 }  

lcd.setCursor(0, 1); lcd.print("Detected ");

  if (CT_count==0) {
    Serial.println("NO CT's detected, sampling from CT1 by default");
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
    Serial.println(" DS18B20..using this for temperature reading");
    delay(5000);
    lcd.clear();
    lcd.print("Detected: "); lcd.print(numSensors); 
    lcd.setCursor(0, 1); lcd.print("DS18B20 Temp"); 
  }
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


void send_emonpi_serial()  //Send emonPi data to Pi serial /dev/ttyAMA0 using struct JeeLabs RF12 packet structure 
{
  byte binarray[sizeof(emonPi)];
  memcpy(binarray, &emonPi, sizeof(emonPi));
    
  Serial.print(' ');
  Serial.print(nodeID);
  for (byte i = 0; i < 4; ++i) {
    Serial.print(' ');
    Serial.print(binarray[i]);
  }
  Serial.println();
  
  delay(10);
}
