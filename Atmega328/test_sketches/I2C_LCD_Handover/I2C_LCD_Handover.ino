//YWROBOT
//Compatible with the Arduino IDE 1.0
//Library version:1.1
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x20 for a 16 chars and 2 line display

const byte emonpi_LED_pin=9;

void setup()
{
  pinMode(emonpi_LED_pin, OUTPUT);
  digitalWrite(emonpi_LED_pin, HIGH);
  
  Serial.begin(57600);
  Serial.println("ATmega328 Startup");
  Serial.println("I2C LCD Hand-over test");
  delay(100);
  
  lcd.init();                      // initialize the lcd 
 
  // Print a message to the LCD.
  lcd.backlight();
  lcd.clear();
  lcd.println("emonPi");
  lcd.println("booting....");
  Wire.endTransmission();   //might not be needed? 
  
  digitalWrite(emonpi_LED_pin, LOW);
}

void loop()
{
  
  digitalWrite(emonpi_LED_pin, HIGH);
  Serial.println("ATmega328 still running!");
  delay(5000);
  digitalWrite(emonpi_LED_pin, LOW);

}
