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
    
  Serial.print(' ');
  Serial.print(nodeID);
  for (byte i = 0; i < 4; ++i) {
    Serial.print(' ');
    Serial.print(binarray[i]);
  }
  Serial.println();
  
  delay(10);
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
    }
  value = top = 0;
}


static byte bandToFreq (byte band) {
  return band == 4 ? RF12_433MHZ : band == 8 ? RF12_868MHZ : band == 9 ? RF12_915MHZ : 0;
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