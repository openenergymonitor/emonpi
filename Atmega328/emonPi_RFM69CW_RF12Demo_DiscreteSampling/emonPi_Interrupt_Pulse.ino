// The interrupt routine - runs each time a falling edge of a pulse is detected
void onPulse()                  
{  
  emonPi.pulseCount++;					//calculate wh elapsed from time between pulses 	
}