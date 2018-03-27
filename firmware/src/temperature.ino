//Setup and for presence of DS18B20, return number of sensors
byte check_for_DS18B20()
{
  sensors.begin();
  numSensors=(sensors.getDeviceCount());
  
  byte j=0;                                        // search for one wire devices and
                                                   // copy to device address arrays.
  while ( (j < numSensors) && (sensors.getAddress(allAddress[j], j)) )  j++;
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

    if ((temp<125.0) && (temp>-55.0))
      return (temp*10);            //if reading is within range for the sensor convert float to int ready to send via RF
  }
  return -55;  // out of range return minimum
}

