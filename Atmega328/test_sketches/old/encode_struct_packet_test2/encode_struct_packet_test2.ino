byte nodeid;
typedef struct {                                                      // RFM12B RF payload datastructure
        int t1;
        int t2;                                                  
} Payload;
Payload emon;

void setup()
{
  Serial.begin(57600);
  
  emon.t1 = 1245;
  emon.t2 = 1845;
  nodeid=5;

}

void loop()
{ 
  
  emon.t1++;
  byte binarray[sizeof(emon)];
  memcpy(binarray, &emon, sizeof(emon));
    
  Serial.print(' ');
  Serial.print(nodeid);
  for (byte i = 0; i < 4; ++i) {
    Serial.print(' ');
    Serial.print((int) binarray[i]);
  }
  Serial.println();
  
  delay(2000);
    
}
