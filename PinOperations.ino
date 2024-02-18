/// Reads the value of the specified pin
int readPin(int pin)
{
  return digitalRead(pin);
}

/// Turns on the ACC LED
void turnOnAccLed()
{
  digitalWrite(LEDPin, HIGH);
  digitalWrite(PowerMOSPin, HIGH);
}

/// Turns off the ACC LED
void turnOffAccLed()
{
  digitalWrite(LEDPin, LOW);
  digitalWrite(RpiPwSwPin,  LOW); //send shutdown to RPI
  while(digitalRead(RpiStPin)) 
    {
      delay(1); //wait for rpi off state
    }
}
