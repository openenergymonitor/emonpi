// emonPi used 16 x 2 I2C LCD display 

/*
void emonPi_LCD_Startup() {
  lcd.init();                      // initialize the lcd 
  lcd.backlight();                 // Or lcd.noBacklight() 
  lcd.print("emonPi V"); lcd.print(firmware_version*0.1);
  lcd.setCursor(0, 1); lcd.print("OpenEnergyMon");
} 
*/
int DS18B20_PWR =19;
void serial_print_startup(){
  //-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//  lcd.clear(); 

  Serial.print("CT 1 Cal: "); Serial.println(Ical1);
  Serial.print("CT 2 Cal: "); Serial.println(Ical2);
  Serial.print("CT 3 Cal: "); Serial.println(Ical3);
  Serial.print("CT 4 Cal: "); Serial.println(Ical4);
  delay(1000);
  Serial.print("VRMS AC ~");
  Serial.print(vrms); Serial.println("V");

  if (ACAC) 
  {
    for (int i=0; i<10; i++)                                              // indicate AC has been detected by flashing LED 10 times
    { 
      digitalWrite(LEDpin, HIGH); delay(200);
      digitalWrite(LEDpin, LOW); delay(300);
    }
//    lcd.print("AC Wave Detected");
    Serial.println("AC Wave Detected - Real Power calc enabled");
    if (USA==TRUE) Serial.print("USA mode active "); 
    Serial.print("Vcal: "); Serial.println(Vcal);
    Serial.print("Vrms: "); Serial.print(Vrms); Serial.println("V");
    Serial.print("Phase Shift: "); Serial.println(phase_shift);
  }
  else 
  {
     delay(1000);
    digitalWrite(LEDpin, HIGH); delay(2000); digitalWrite(LEDpin, LOW);   // indicate DC power has been detected by turing LED on then off
  
//   lcd.print("AC NOT Detected");
   Serial.println("AC NOT detected - Apparent Power calc enabled");
   if (USA==TRUE) Serial.println("USA mode"); 
   Serial.print("Assuming VRMS: "); Serial.print(Vrms); Serial.println("V");
   Serial.println("Assuming power from batt / 5V USB - power save enabled");
 }  

//lcd.setCursor(0, 1); lcd.print("Detected ");

  if (CT_count==0) {
    Serial.println("no CT detected");
//    lcd.print("No CT's");
  }
   else   
   {
    
     if (CT1) {
      Serial.println("CT 1 detect");
//      lcd.print("CT1 ");
    }
     if (CT2) {
      Serial.println("CT 2 detect");
//      lcd.print("CT2");
    }
     if (CT3) {
      Serial.println("CT 3 detect");
//      lcd.print("CT3");
    }
     if (CT4) {
      Serial.println("CT 4 detect");
//      lcd.print("CT4");
    }
   }
  
  delay(2000);

  if (DS18B20_STATUS==1) {
    Serial.print("Detect "); 
    Serial.print(numSensors); 
    Serial.println(" DS18B20");
//    lcd.clear();
//    lcd.print("Detected: "); lcd.print(numSensors); 
//    lcd.setCursor(0, 1); lcd.print("DS18B20 Temp"); 
  }
  else {
  	Serial.println("0 DS18B20 detected");
//  	lcd.clear();
//  	lcd.print("Detected: "); lcd.print(numSensors); 
//    lcd.setCursor(0, 1); lcd.print("DS18B20 Temp"); 
  }

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

    Serial.print("CT1 CT2 CT3 CT4 VRMS/BATT PULSE");
    if (DS18B20_STATUS==1){Serial.print(" Temperature 1-"); Serial.print(numSensors);}
    Serial.println(" "); 
    delay(500); 

    showString(helpText1);
  }
  delay(20);  
}


void send_emonpi_serial()  //Send emonPi data to Pi serial /dev/ttyAMA0 using struct JeeLabs RF12 packet structure 
{
  byte binarray[sizeof(emonPi)];
  memcpy(binarray, &emonPi, sizeof(emonPi));
  
  Serial.print("OK ");
  Serial.print(nodeID);
  //---------------------------------------remote control------------------------------------------------------------------------------------------------------------
  
         //Serial.print(" Power1 is[");
         //Serial.print(emonPi.power1);
         //Serial.print("]");
         if (emonPi.v_battery_bank < 24)
         {
          //Serial.println("load_monitor is ON");
           switch_on_relay1();   
         }
         else
         {
          
          switch_off_relay1();  
         }
    //---------------------------------------
         if (emonPi.power1 < 10)
         {
          //Serial.println("load_monitor is ON");
           switch_on_relay2();   
         }
         else
         {
          
          switch_off_relay2();  
         }
      //---------------------------------------------------

          if (emonPi.power2 < 10)
         {
          //Serial.println("load_monitor is ON");
           switch_on_relay3();   
         }
         else
         {
          
          switch_off_relay3();  
         }
   //------------------------------------------------------------------------------------------------------------------------------------
            if (emonPi.power3 < 10)
         {
          //Serial.println("load_monitor is ON");
           switch_on_relay4();   
         }
         else
         {
          
          switch_off_relay4();  
         }
         //----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

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
//---------------------------------------ON/OFF relay driver--------------------------------------------------------
//------------------------relay1------------------------------------
void switch_on_relay1()
{
 // digitalWrite(DS18B20_PWR,HIGH);
  load_monitor.relay1 = 1;
  rf12_sleep(RF12_WAKEUP);                                   
  rf12_sendNow(0, &load_monitor, sizeof load_monitor);                           //send temperature data via RFM12B using new rf12_sendNow wrapper
  rf12_sendWait(.5);
  //Serial.print("load_monitor is ON");
}

void switch_off_relay1()
{
  //digitalWrite(DS18B20_PWR,LOW);
  //Serial.print("load_monitor is OFF");
  load_monitor.relay1 = 0;
  rf12_sleep(RF12_WAKEUP);                                   
  rf12_sendNow(0, &load_monitor, sizeof load_monitor);                           //send temperature data via RFM12B using new rf12_sendNow wrapper
  rf12_sendWait(.5);
}
//------------------------pv productiont------------------------------------
void switch_on_relay2()
{
 // digitalWrite(DS18B20_PWR,HIGH);
  load_monitor.relay2 = 1;
  rf12_sleep(RF12_WAKEUP);                                   
  rf12_sendNow(0, &load_monitor, sizeof load_monitor);                           //send temperature data via RFM12B using new rf12_sendNow wrapper
  rf12_sendWait(.5);
  //Serial.print("load_monitor is ON");
}

void switch_off_relay2()
{
  //digitalWrite(DS18B20_PWR,LOW);
  //Serial.print("load_monitor is OFF");
  load_monitor.relay2 = 0;
  rf12_sleep(RF12_WAKEUP);                                   
  rf12_sendNow(0, &load_monitor, sizeof load_monitor);                           //send temperature data via RFM12B using new rf12_sendNow wrapper
  rf12_sendWait(.5);
}
//-----------------------load shedding 1------------------------------------
void switch_on_relay3()
{
 // digitalWrite(DS18B20_PWR,HIGH);
  load_monitor.relay3 = 1;
  rf12_sleep(RF12_WAKEUP);                                   
  rf12_sendNow(0, &load_monitor, sizeof load_monitor);                           //send temperature data via RFM12B using new rf12_sendNow wrapper
  rf12_sendWait(.5);
  //Serial.print("load_monitor is ON");
}

void switch_off_relay3()
{
  //digitalWrite(DS18B20_PWR,LOW);
  //Serial.print("load_monitor is OFF");
  load_monitor.relay3 = 0;
  rf12_sleep(RF12_WAKEUP);                                   
  rf12_sendNow(0, &load_monitor, sizeof load_monitor);                           //send temperature data via RFM12B using new rf12_sendNow wrapper
  rf12_sendWait(.5);
}
//------------------------load shedding 2------------------------------------
void switch_on_relay4()
{
 // digitalWrite(DS18B20_PWR,HIGH);
  load_monitor.relay4 = 1;
  rf12_sleep(RF12_WAKEUP);                                   
  rf12_sendNow(0, &load_monitor, sizeof load_monitor);                           //send temperature data via RFM12B using new rf12_sendNow wrapper
  rf12_sendWait(.5);
  //Serial.print("load_monitor is ON");
}

void switch_off_relay4()
{
  //digitalWrite(DS18B20_PWR,LOW);
  //Serial.print("load_monitor is OFF");
 load_monitor.relay4 = 0;
  rf12_sleep(RF12_WAKEUP);                                   
  rf12_sendNow(0, &load_monitor, sizeof load_monitor);                           //send temperature data via RFM12B using new rf12_sendNow wrapper
  rf12_sendWait(.5);
}


