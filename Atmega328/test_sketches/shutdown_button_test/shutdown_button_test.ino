
const byte emonpi_LED_pin=9;
const byte shutdown_switch_pin = 8;



void setup()
{
  pinMode(emonpi_LED_pin, OUTPUT);
  digitalWrite(emonpi_LED_pin, HIGH);
  Serial.begin(57600);
  Serial.println("ATmega328 Startup");
  Serial.println("emon Pi Shutdown Button test");
  delay(100);
  
  pinMode(shutdown_switch_pin,INPUT_PULLUP);            //enable ATmega328 internal pull-up resistors 
  
  
}

void loop()
{
  
  if (digitalRead(shutdown_switch_pin == LOW) ){
    Serial.print("Shutdown!");
    Serial.println("sudo halt");
    digitalWrite(emonpi_LED_pin, HIGH);
    delay(5000);
  }
  

  digitalWrite(emonpi_LED_pin, LOW);
}
    
