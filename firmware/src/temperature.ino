//Setup and for presence of DS18B20, return number of sensors
byte check_for_DS18B20()
{
        sensors.begin();
        numSensors = min(MaxOnewire, sensors.getDeviceCount());  // If more sensors are detected than allowed only read from max allowed number

        byte j = 0;                                  // search for one wire devices and
                                                   // copy to device address arrays.
        while ((j < numSensors) && (sensors.getAddress(allAddress[j], j)))
            j++;
        for (byte j = 0; j < numSensors; j++)
            sensors.setResolution(allAddress[j], TEMPERATURE_PRECISION); // and set the a to d conversion resolution of each.
        return numSensors;
}

int get_temperature(byte sensor)
{
        float temp = sensors.getTempC(allAddress[sensor]);

        if ((temp < 125.0) && (temp > -55.0))
                return temp * 10; //if reading is within range for the sensor convert float to int ready to send via RF

        return -100; // How to signal invalid?
}
