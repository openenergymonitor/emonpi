


void shutdown_sequence()
  {
    Serial.println("Hold to shutdown...release to reset MCU"); 
    lcd.clear();
    lcd.print("RESET"); lcd.setCursor(0, 1);
    lcd.print("Hold to Shutdown");
    digitalWrite(LEDpin, HIGH);
    
    start_press=millis();                                                                 // record time shutdown push button is pressed
    byte flag=0;
    while( ((millis()-start_press) < 5000))                                               // time 5s
    {
      if  (digitalRead(shutdown_switch_pin) == 1)                                         // if shutdown button is released in less than 5s then soft restart ATmega328 
        asm volatile ("  jmp 0");                                                         // restarts sketch but does not reset the peripherals and registers
      
      if ((millis()-start_press >1000) && (flag==0)) 
      {
        Serial.println("...5"); 
        flag=1;
         lcd.clear();
        lcd.print("5s to shutdown"); lcd.setCursor(0, 1);
        lcd.print("Release to reset");
      }

      if ((millis()-start_press >2000) && (flag==1)) 
      {
        Serial.println("...4"); 
        flag=2;
         lcd.clear();
        lcd.print("4s to shutdown"); lcd.setCursor(0, 1);
        lcd.print("Release to reset");
      }

      if ((millis()-start_press >3000) && (flag==2)) 
      {
        Serial.println("...3"); 
        flag=3;
         lcd.clear();
        lcd.print("3s to shutdown"); lcd.setCursor(0, 1);
        lcd.print("Release to reset");
      }
      
      if ((millis()-start_press >4000) && (flag==3)) 
      {
        Serial.println("...2"); 
        flag=4;
         lcd.clear();
        lcd.print("2s to shutdown"); lcd.setCursor(0, 1);
        lcd.print("Release to reset");
      }

      if ((millis()-start_press >4500) && (flag==4)) 
      {
        lcd.print("1s to shutdown"); lcd.setCursor(0, 1);
        lcd.print("Release to reset");
        Serial.println("...1"); 
        Serial.println("SHUTDOWN!");
        flag=5;
      }
    }
     digitalWrite(emonpi_GPIO_pin, HIGH);                                                  // sent shutdown signal to Pi GPIO pin after 5s
     
     lcd.clear();
     lcd.print("RaspberryPi"); lcd.setCursor(0, 1);
     lcd.print("Shutdown NOW!");
     delay(5000);
     lcd.noBacklight();
     lcd.clear(); lcd.print("Power Off");
  
}

