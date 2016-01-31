// emonPi used 16 x 2 I2C LCD display 

void emonPi_LCD_Startup() {
  lcd.init();                      // initialize the lcd 
  lcd.backlight();                 // Or lcd.noBacklight() 
  lcd.print(F("emonPi V")); lcd.print(firmware_version*0.1);
  lcd.setCursor(0, 1); lcd.print(F("OpenEnergyMon"));
} 

void serial_print_startup(){
  //-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  lcd.clear(); 

  Serial.print(F("CT 1 Cal: ")); Serial.println(Ical1);
  Serial.print(F("CT 2 Cal: ")); Serial.println(Ical2);
  Serial.print(F("VRMS AC ~"));
  Serial.print(vrms); Serial.println(F("V"));

  if (ACAC) 
  {
    lcd.print(F("AC Wave Detected"));
    Serial.println(F("AC Wave Detected - Real Power calc enabled"));
    if (USA==TRUE) Serial.print(F("USA mode > ")); 
    Serial.print(F("Vcal: ")); Serial.println(Vcal);
    Serial.print(F("Vrms: ")); Serial.print(Vrms); Serial.println(F("V"));
  }
  else 
  {
   lcd.print(F("AC NOT Detected"));
   Serial.println(F("AC NOT detected - Apparent Power calc enabled"));
   if (USA==TRUE) Serial.println(F("USA mode")); 
   Serial.print(F("Assuming VRMS: ")); Serial.print(Vrms); Serial.println(F("V"));
 }  

lcd.setCursor(0, 1); lcd.print(F("Detected "));

  if (CT_count==0) {
    Serial.println(F("no CT detected"));
    lcd.print(F("No CT's"));
  }
   else   
   {
    
     if (CT1) {
      Serial.println(F("CT 1 detect"));
      lcd.print(F("CT1 "));
    }
     if (CT2) {
      Serial.println(F("CT 2 detect"));
      lcd.print(F("CT2"));
    }
   }
  
  delay(2000);

  if (DS18B20_STATUS==1) {
    Serial.print(F("Detect ")); 
    Serial.print(numSensors); 
    Serial.println(F(" DS18B20"));
    lcd.clear();
    lcd.print(F("Detected: ")); lcd.print(numSensors); 
    lcd.setCursor(0, 1); lcd.print(F("DS18B20 Temp")); 
  }
  else {
  	Serial.println(F("0 DS18B20 detected"));
  	lcd.clear();
  	lcd.print(F("Detected: ")); lcd.print(numSensors); 
    lcd.setCursor(0, 1); lcd.print(F("DS18B20 Temp")); 
  }
  
  delay(2000);
  
  lcd.clear();
  lcd.print(F("Raspberry Pi"));
  lcd.setCursor(0, 1); lcd.print(F("Booting...")); 
   
   

  if (RF_STATUS == 1){
    #if (RF69_COMPAT)
      Serial.println(F("RFM69CW Init: "));
    #else
      Serial.println(F("RFM12B Init: "));
    #endif

    Serial.print(F("Node ")); Serial.print(nodeID); 
    Serial.print(F(" Freq ")); 
    if (RF_freq == RF12_433MHZ) Serial.print(F("433Mhz"));
    if (RF_freq == RF12_868MHZ) Serial.print(F("868Mhz"));
    if (RF_freq == RF12_915MHZ) Serial.print(F("915Mhz")); 
    Serial.print(F(" Network ")); Serial.println(networkGroup);

    showString(helpText1);
  }
  delay(20);  
}


void send_emonpi_serial()  //Send emonPi data to Pi serial /dev/ttyAMA0 using struct JeeLabs RF12 packet structure 
{
  byte binarray[sizeof(emonPi)];
  memcpy(binarray, &emonPi, sizeof(emonPi));
  
  Serial.print(F("OK "));
  Serial.print(nodeID);
  for (byte i = 0; i < sizeof(binarray); i++) {
    Serial.print(F(" "));
    Serial.print(binarray[i]);
  }
  Serial.print(F(" (-0)"));
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
