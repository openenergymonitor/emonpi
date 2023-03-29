//Setup and for presence of DS18B20, return number of sensors
byte check_for_DS18B20()
{
  sensors.begin();
  numSensors = (sensors.getDeviceCount());

  byte j = 0;                                      // search for one wire devices and
  // copy to device address arrays.
  while ( (j < numSensors) && (sensors.getAddress(allAddress[j], j)) )  j++;
  for (byte j = 0; j < numSensors; j++) sensors.setResolution(allAddress[j], TEMPERATURE_PRECISION); // and set the a to d conversion resolution of each.

  if (numSensors == 0) DS18B20_STATUS = 0;
  else DS18B20_STATUS = 1;

  if (numSensors > MaxOnewire) numSensors = MaxOnewire; //If more sensors are detected than allowed only read from max allowed number

  return numSensors;
}

int get_temperature(byte sensor) {

  float temp = 300 ;
  /*
    if (DEBUG) {
    Serial.print("Sensor address = " );
    for(int x=8; x>0 ; x--){
      Serial.print(allAddress[sensor][x], HEX) ;
      Serial.print(" ") ;
    }
    Serial.println();
    }
  */
  if (allAddress[sensor][0] == 0x26) {
    DS2438 ds2438(&oneWire, allAddress[sensor]);
    ds2438.begin() ;
    ds2438.update() ;
    if (ds2438.isError()) {
      //Serial.println("Error reading from DS2438 device");
      temp = -55 ;
    } else {
      temp = ds2438.getTemperature() ;
      /*
      if (DEBUG) {
        Serial.print("DS2438 Temperature = ");
        Serial.print(temp, 1);
        Serial.println() ;
      }
      */
    }
  } else if (allAddress[sensor][0] == 0x28) {

    temp = (sensors.getTempC(allAddress[sensor]));
    /*
    if (DEBUG) {
      Serial.print("DS18x20 Temperature = ");
      Serial.print(temp, 1);
      Serial.println() ;
    }
    */
  }

  if ((temp < 125.0) && (temp > -55.0)) {
    return (temp * 10);         //if reading is within range for the sensor convert float to int ready to send via RF
  } else {
    return -55;  // out of range return minimum
  }

}

