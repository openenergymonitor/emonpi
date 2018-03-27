void RF_Setup(){
	//--------------------------------------------------Initalize RF and send out RF test packets--------------------------------------------------------------------------------------------  
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
}

bool RF_Rx_Handle(){

  if (rf12_recvDone()) {		//if RF Packet is received
    if (rf12_crc == 0) {		//Check packet is good
      Serial.print(F("OK"));		//Print "good packet" line prefix
      print_frame(rf12_len);		//Print recieved data
      if (RF12_WANTS_ACK==1) {
        // Serial.print(F(" -> ack"));
        rf12_sendStart(RF12_ACK_REPLY, 0, 0);
      }
      return true;
    } else {
      if (quiet_mode == 0) {            //if the packet is bad
        Serial.print(F(" ?"));    	//Print the "bad packet" line prefix
        print_frame(20);          	//Print only the first 20 bytes of a bad packet
      }
    }
  } //end recDone
  return false;
}

void print_frame (int len) {
    Serial.print(F(" "));
    Serial.print(rf12_hdr & 0x1F);        // Extract and print node ID
    Serial.print(F(" "));
    for (byte i = 0; i < len; ++i) {
        Serial.print((word)rf12_data[i]);
        Serial.print(F(" "));
    }
    #if RF69_COMPAT
    // display RSSI value after packet data e.g (-xx)
    Serial.print(F("("));
    Serial.print(-(RF69::rssi>>1));
    Serial.print(F(")"));
    #endif
    Serial.println();
}

void send_RF(){

	if (cmd && rf12_canSend() ) {                                                //if command 'cmd' is waiting to be sent then let's send it
	    digitalWrite(LEDpin, HIGH); delay(200); digitalWrite(LEDpin, LOW);
	    showString(PSTR(" -> "));
	    Serial.print((word) sendLen);
	    showString(PSTR(" b\n"));
	    byte header = cmd == 'a' ? RF12_HDR_ACK : 0;
	    if (dest)
	      header |= RF12_HDR_DST | dest;
	    rf12_sendStart(header, stack, sendLen);
	    cmd = 0;
	    
	}
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
        if (value>=0){
          networkGroup = value;
          if (RF_STATUS==1) rf12_initialize(nodeID, RF_freq, networkGroup);
        }
        break;

      case 'p': // set Vcc Cal 1=UK/EU 2=USA
        if (value){
          if (value==1) USA=false;
          if (value==2) USA=true;
        }
        break;

      case 'q': // turn quiet mode on or off (don't report bad packets)
        quiet_mode = value;
        break;

      case 'v': // print firmware version
        Serial.print(F("[emonPi.")); Serial.print(firmware_version*0.1); Serial.print(F("]"));
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

    if (RF_STATUS==1) {
      Serial.print(F(" "));
      Serial.print((char) ('@' + (nodeID & RF12_HDR_MASK)));
      Serial.print(F(" i"));
      Serial.print(nodeID & RF12_HDR_MASK);
      Serial.print(F(" g"));
      Serial.print(networkGroup);
      Serial.print(F(" @ "));
      Serial.print(RF_freq == RF12_433MHZ ? 433 :
                   RF_freq == RF12_868MHZ ? 868 :
                   RF_freq == RF12_915MHZ ? 915 : 0);
      Serial.print(F(" MHz"));
      Serial.print(F(" q")); 
      Serial.print(quiet_mode);
    }
    Serial.print(F(" USA ")); Serial.print(USA);
    Serial.println(F(" "));
    
    }
  value = top = 0;
}


static byte bandToFreq (byte band) {
  return band == 4 ? RF12_433MHZ : band == 8 ? RF12_868MHZ : band == 9 ? RF12_915MHZ : 0;
}
 
