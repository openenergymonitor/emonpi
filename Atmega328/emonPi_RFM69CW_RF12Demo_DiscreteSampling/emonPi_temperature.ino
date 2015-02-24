
int check_for_DS18B20()                                      //Setup and for presence of DS18B20, return number of sensors 
{
  sensors.begin();
  sensors.setWaitForConversion(false);             //disable automatic temperature conversion to reduce time spent awake, conversion will be implemented manually in sleeping http://harizanov.com/2013/07/optimizing-ds18b20-code-for-low-power-applications/ 
  numSensors=(sensors.getDeviceCount()); 
  
  byte j=0;                                        // search for one wire devices and
                                                   // copy to device address arrays.
  while ((j < numSensors) && (oneWire.search(allAddress[j])))  j++;
  
  if (numSensors==0) DS18B20_STATUS=0; 
    else DS18B20_STATUS=1;
    
 return numSensors;   
}

int get_temperature()                //requ
{
    {
    
     for(int j=0;j<numSensors;j++) sensors.setResolution(allAddress[j], TEMPERATURE_PRECISION);      // and set the a to d conversion resolution of each.
     sensors.requestTemperatures();                                        // Send the command to get temperatures
     Sleepy::loseSomeTime(ASYNC_DELAY); //Must wait for conversion, since we use ASYNC mode
     float temp=(sensors.getTempC(allAddress[0]));
   
     if ((temp<125.0) && (temp>-40.0)) return(temp*10);            //if reading is within range for the sensor convert float to int ready to send via RF
     //if (debug==1) {Serial.print("temperature: "); Serial.println(emonPi.temp*0.1); delay(20);}
  }
}

