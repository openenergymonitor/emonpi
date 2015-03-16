 
double calc_rms(int pin, int samples)                      //Used in emonPi startup to detect presence of AC wavefrom and estimate VRMS voltage
{
  unsigned long sum = 0;
  for (int i=0; i<samples; i++) // 178 samples takes about 20ms
  {
    int raw = (analogRead(0)-512);
    sum += (unsigned long)raw * raw;
  }
  double rms = sqrt((double)sum / samples);
  return rms;
}

void emonPi_startup()                                                     //emonPi startup proceadure, check for AC waveform and print out debug
{
  pinMode(LEDpin, OUTPUT); 
  digitalWrite(LEDpin,HIGH); 

  pinMode(shutdown_switch_pin,INPUT_PULLUP);            //enable ATmega328 internal pull-up resistors 

  pinMode(emonpi_GPIO_pin, OUTPUT);                     //Connected to RasPi GPIO pin 17
  digitalWrite(emonpi_GPIO_pin, LOW);
  
  pinMode(emonPi_int1, INPUT);                          // Set RJ45 interrupt pin to input (INT 1)

  Serial.begin(BAUD_RATE);
  Serial.print("emonPi V"); Serial.print(firmware_version); 
  Serial.println("OpenEnergyMonitor.org");
  Serial.println("please wait.....");
}


 
void CT_Detect(){
//--------------------------------------------------Check for connected CT sensors--------------------------------------------------------------------------------------------------------- 
if (analogRead(1) > 0) {CT1 = 1; CT_count++;} else CT1=0;              // check to see if CT is connected to CT1 input, if so enable that channel
if (analogRead(2) > 0) {CT2 = 1; CT_count++;} else CT2=0;              // check to see if CT is connected to CT2 input, if so enable that channel
if ( CT_count == 0) CT1=1;                                                                        // If no CT's are connected then by default read from CT1
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//--------------------------------------------------Check for connected AC Adapter Sensor------------------------------------------------------------------------------------------------

// Quick check to see if there is a voltage waveform present on the ACAC Voltage input
// Check consists of calculating the RMS from 100 samples of the voltage input.
delay(5000);
digitalWrite(LEDpin,LOW); 

// Calculate if there is an ACAC adapter on analog input 0
vrms = calc_rms(0,1780) * 0.87;      //ADC 0   double vrms = calc_rms(0,1780) * (Vcal * (3.3/1024) );
if (vrms>90) ACAC = 1; else ACAC=0;
//Serial.print(vrms);
}


