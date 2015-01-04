void send_emonpi_serial()  //Send emonPi data to Pi serial /dev/ttyAMA0 using struct packet structure 
{
  byte binarray[sizeof(emonPi)];
  memcpy(binarray, &emonPi, sizeof(emonPi));
    
  Serial.print(' ');
  Serial.print(nodeID);
  for (byte i = 0; i < 4; ++i) {
    Serial.print(' ');
    Serial.print((int) binarray[i]);
  }
  Serial.println();
  
  delay(10);
}
