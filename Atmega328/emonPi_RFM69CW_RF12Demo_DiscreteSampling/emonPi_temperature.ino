
byte check_for_DS18B20()                                      //Setup and for presence of DS18B20, return number of sensors 
{
  //digitalWrite(DS18B20_PWR, HIGH); delay(100); 
  sensors.begin();
  sensors.setWaitForConversion(false);             // disable automatic temperature conversion to reduce time spent awake, conversion will be implemented manually in sleeping 
                                                   // http://harizanov.com/2013/07/optimizing-ds18b20-code-for-low-power-applications/ 
  numSensors=(sensors.getDeviceCount());
//  if (numSensors > MaxOnewire) numSensors=MaxOnewire;   //Limit number of sensors to max number of sensors 
  
  numSensors=(sensors.getDeviceCount()); 
  
  byte j=0;                                        // search for one wire devices and
                                                   // copy to device address arrays.
  while ((j < numSensors) && (oneWire.search(allAddress[j])))  j++;
  delay(500);
  //digitalWrite(DS18B20_PWR, LOW);
  for(byte j=0;j<numSensors;j++) sensors.setResolution(allAddress[j], TEMPERATURE_PRECISION);      // and set the a to d conversion resolution of each.
  
  if (numSensors==0) DS18B20_STATUS=0; 
    else DS18B20_STATUS=1;

  if (numSensors>MaxOnewire) numSensors=MaxOnewire;     //If more sensors are detected than allowed only read from max allowed number
    
 return numSensors;   
}

int get_temperature(byte sensor)                
{
    {
    
     float temp=(sensors.getTempC(allAddress[sensor]));
   
     if ((temp<125.0) && (temp>-55.0)) return(temp*10);            //if reading is within range for the sensor convert float to int ready to send via RF
  }
}

