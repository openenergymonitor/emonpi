#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 4                         //emonPi RJ45 DS18B20 one-wire 
#define TEMPERATURE_PRECISION 12

const byte emonpi_LED_pin=9;
const byte shutdown_switch_pin = 8;
const byte emonpi_GPIO_pin=5;              //connected to Pi GPIO 17

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);



byte num_sensors; 
long unsigned int start_press;
byte flag;
DeviceAddress tmp_address;


void setup(void)
{
  pinMode(emonpi_LED_pin, OUTPUT);
  digitalWrite(emonpi_LED_pin, HIGH);
  
  pinMode(shutdown_switch_pin, INPUT);
  pinMode(shutdown_switch_pin,INPUT_PULLUP);            //enable ATmega328 internal pull-up resistors 
  
  Serial.begin(57600);
  Serial.println("ATmega328 Startup");
  Serial.println("Dallas Temperature IC Control Library Demo");

  sensors.begin();

  Serial.println("Locating devices...");
  delay(1000);
  Serial.print("found: ");
  num_sensors = (sensors.getDeviceCount());
  Serial.print(num_sensors);
  Serial.println(" devices.");
  
  digitalWrite(emonpi_LED_pin, LOW);
}



void loop()
{ 
  digitalWrite(emonpi_LED_pin, HIGH);
  sensors.requestTemperatures();
  
   Serial.print("Found ");  Serial.print(num_sensors ); Serial.println(" devices.");
 
   for(int i=0;i<num_sensors; i++)
  {
    sensors.getAddress(tmp_address, i);
    printAddress(tmp_address);
    Serial.println();
  }
  
  if (num_sensors >= 0) 
    take_ds18b20_reading();

  delay(5000);
  digitalWrite(emonpi_LED_pin, LOW);
  if (digitalRead(shutdown_switch_pin) == 0 ) shutdown_sequence(); 
  
 delay(5000);
}


void printAddress(DeviceAddress deviceAddress)
{
  Serial.print("{ ");
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    Serial.print("0x");
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
    if (i<7) Serial.print(", ");
  }
  Serial.print(" }");
}  
  
void take_ds18b20_reading()   
{
  // Loop through each device, print out temperature data
  for(int i=0;i<num_sensors; i++)
  {
    // Search the wire for address
    if(sensors.getAddress(tmp_address, i))
      {	
           sensors.setResolution(tmp_address, TEMPERATURE_PRECISION);
           
           // Get readings. We must wait for ASYNC_DELAY due to power-saving (waitForConversion = false)
           sensors.requestTemperatures();                                   
           float temp1=(sensors.getTempC(tmp_address));
           
           // Payload will maintain previous reading unless the temperature is within range.
           if (temperature_in_range(temp1)){
           Serial.print(i); Serial.print(": "); Serial.println(temp1);
	   } 
          else 
          Serial.print(i); Serial.print(": ");  Serial.println("ghost device! Check your power requirements and cabling"); 
	//else ghost device! Check your power requirements and cabling
      }
  }
}
   



boolean temperature_in_range(float temp)
{
  // Only accept the reading if it's within a desired range.
  float minimumTemp = -40.0;
  float maximumTemp = 125.0;

  return temp > minimumTemp && temp < maximumTemp;
}


 void shutdown_sequence()
  {
    Serial.println("Hold button for 5s to shutdown Pi...release to restart just emonPi ATmega328"); 
    digitalWrite(emonpi_LED_pin, HIGH);
    
    start_press=millis();                                                                 // record time shutdown push button is pressed
    flag=0;
    while( ((millis()-start_press) < 5000))                                               // time 5s
    {
      if  (digitalRead(shutdown_switch_pin) == 1)                                         // if shutdown button is released in less than 5s then soft restart ATmega328 
        asm volatile ("  jmp 0");                                                         // restarts sketch but does not reset the peripherals and register
    }
    digitalWrite(emonpi_GPIO_pin, HIGH);                                                  // sent shutdown signal to Pi GPIO pin after 5s
    asm volatile ("  jmp 0");                                                             // soft restart ATmega328    
}

