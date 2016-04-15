
const byte emonpi_LED_pin=9;
const byte shutdown_switch_pin = 8;
const byte emonpi_GPIO_pin=5;              //connected to Pi GPIO 17

long unsigned int start_press=0; 
byte flag=0;



void setup()
{
  pinMode(emonpi_LED_pin, OUTPUT);
  digitalWrite(emonpi_LED_pin, HIGH);
  Serial.begin(57600);
  Serial.println("ATmega328 Startup");
  Serial.println("emon Pi Shutdown Button test");
  delay(100);
  
  pinMode(shutdown_switch_pin, INPUT);
  pinMode(shutdown_switch_pin,INPUT_PULLUP);            //enable ATmega328 internal pull-up resistors 
  
  pinMode(emonpi_GPIO_pin, OUTPUT);
  digitalWrite(emonpi_GPIO_pin, LOW);
  
}

void loop()
{
  
  if (digitalRead(shutdown_switch_pin) == 0 ) shutdown_sequence(); 
  
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
        asm volatile ("  jmp 0");                                                         // restarts sketch but does not reset the peripherals and registers
      
      if ((millis()-start_press >1000) && (flag==0)) 
      {
        Serial.println("...5"); 
        flag=1;
      }

      if ((millis()-start_press >2000) && (flag==1)) 
      {
        Serial.println("...4"); 
        flag=2;
      }

      if ((millis()-start_press >3000) && (flag==2)) 
      {
        Serial.println("...3"); 
        flag=3;
      }
      
      if ((millis()-start_press >4000) && (flag==3)) 
      {
        Serial.println("...2"); 
        flag=4;
      }

      if ((millis()-start_press >4500) && (flag==4)) 
      {
        Serial.println("...1"); 
        Serial.println("SHUTDOWN!");
        flag=5;
      }
    }
    digitalWrite(emonpi_GPIO_pin, HIGH);                                                  // sent shutdown signal to Pi GPIO pin after 5s
    asm volatile ("  jmp 0");                                                             // soft restart ATmega328    
}
