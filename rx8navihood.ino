/* 
 Navi
 
 Controls the motor and sensor of the navigation display on the 2001 to 2008 Mazda 6. 
 Replacing the stock screen controller functionality with a customizable one.
 
 This is useful if you want to use the stock navigation enclosure with a tablet and want 
 to retain the same buttons and functionality of the original screen.
 
 This code can be used for RX-8 as well. ( modify tresholds for open, closed and tilted positions)
 
 The circuit:
 * 2 pushbuttons attached from pin 2 and 3 to GND (use existing buttons on Mazda 6 input screen / RX-8 buttons)
 * 1 BA6209 H-Bridge for controlling the motor with its inputs attached to pin 4 and 5.
 * 1 Potentiometer to control position. (included in stock nav enclosure) conected to analog pin 0
 * 1 Motor (included in stock nav enclosure)
 
 created 27 January 2024
 by Raphael PEREIRA
 
 Based on code by Alejandro Mora (27/10/2014)
 */
 
// Includes
#include <EEPROM.h>
#include "functions.h"

#define DEBUG 1

/// Constants.
const int SensorPin       = A0;  //pot sensor navhood position
const int SensorRefPin    = 2;  // pot sensor +5v from digitalpin arduino
const int ForwardPin      = 3;  //navhood motor pin
const int BackwardsPin    = 4;  //navhood motor pin
const int EnaPin          = 5;  //L298N enable pin (h-bridge)
const int TiltPin         = 8; //tilt button
const int OpenClosePin    = 9;  //open/close button
const int AccessoryPin    = 12; //get 12Vacc signal from optoisolator
const int LEDPin          = 13; //just for debug after we need to deactivate

const int PowerMOSPin     = 6;  //pin for activate IRF520 switch on/off 12v powersupply (safeshutdown)
const int RpiPwSwPin      = 10;  //output pin for send shutdown command to rpi GPIO16
const int RpiStPin        = 11; //input pin to get Rpi On/off state to RPI GPIO26

// Handles Status
CurrentStatus currentStatus;

void setup() {
  Serial.begin(57600);
  serialPrintLn("Navi: Mazda RX-8 navigation screen controller");
  
  /// Default status variables.
  currentStatus.oldLidStatus = ClosedStatus;
  currentStatus.lidStatus = ClosedStatus;
  currentStatus.oldAccessoryState = LOW;
  currentStatus.accessoryState = LOW;
  currentStatus.openCloseButtonState = HIGH;
  currentStatus.tiltButtonState = HIGH;

  // Set up pins
  pinMode(ForwardPin, OUTPUT);
  pinMode(BackwardsPin, OUTPUT);  
  pinMode(EnaPin, OUTPUT);  
  pinMode(LEDPin, OUTPUT);
  pinMode(OpenClosePin, INPUT_PULLUP);
  pinMode(TiltPin, INPUT_PULLUP);
  pinMode(AccessoryPin, INPUT);   //+5VACC
  pinMode(SensorPin, INPUT); //analog pin for pot value
  pinMode(SensorRefPin, OUTPUT); //must be high for sensing pot
  
  pinMode(PowerMOSPin, OUTPUT); //stay on if acc swithc off
  pinMode(RpiPwSwPin, OUTPUT); //put LOW to send shutdown RpiStPin
  pinMode(RpiStPin, INPUT); //high when Rpi is ON

  // loads previous status from EEPROM memory
  currentStatus.oldLidStatus = loadPreviousStatus();
  digitalWrite(LEDPin, LOW);
  digitalWrite(SensorRefPin, HIGH); //activate trimpot ref
  digitalWrite(RpiPwSwPin, HIGH); //RPI ON state with lowering voltage to 3.3V
}

void loop() {  
  // stop all movement;
  stopMovement();
  
  // Poll ACC State
  currentStatus.accessoryState = readPin(AccessoryPin);  
  if(currentStatus.accessoryState == HIGH)
  {
    // ACC On
    turnOnAccLed();
    // Poll Buttons
    currentStatus.openCloseButtonState = readPin(OpenClosePin);
    currentStatus.tiltButtonState = readPin(TiltPin);
    if(currentStatus.openCloseButtonState == LOW)
    {
      serialPrintLn("Accessory Power: ON ");
      serialPrintLn("Open/Close button pressed...");
      
      if(isLidClosed())
      {
        // if lid is closed, open it.        
        currentStatus.lidStatus = openLid(OpenStatus);
      }
      else 
      {  
        // else is open or tilted, close it.      
        currentStatus.lidStatus = closeLid();
      }
    }
    
    if(currentStatus.tiltButtonState == LOW && isLidOpen())
    {
      serialPrintLn("Accessory Power: ON ");
      serialPrintLn("Tilt button pressed...");
      // if lid is open, tilt it.
      currentStatus.lidStatus = tiltLid(currentStatus.lidStatus);      
      delay(250); // Add small delay to prevent double triggering. (poor's man debouncer)
    }
  }  
  idle();
}

// When ignition is turned on check for previous state, if the screen was opened
// then open it again, otherwise leave it closed. If the ignition is turned off,
// close the lid
void idle()
{
  // check if ACC is different from previous status
  if(currentStatus.oldAccessoryState != currentStatus.accessoryState)
  {    
    if(currentStatus.accessoryState == HIGH)
    {
      serialPrintLn("Accessory Power: ON ");
      // Ignition is on
      if(currentStatus.oldLidStatus == Tilt0Status ||
         currentStatus.oldLidStatus == Tilt1Status ||
         currentStatus.oldLidStatus == Tilt2Status ||
         currentStatus.oldLidStatus == OpenStatus)
      {
        delay(3000);
        serialPrintLn("Previous lid state: opened...");
        // If car is turned on and lid was open before, open it again.
        currentStatus.lidStatus = openLid(currentStatus.oldLidStatus);
      }
    }
    else
    {
      // Wait a bit to confirm the car is really off instead of being started. (when cranking ACC goes 0v for some seconds)      
      delay(3000);
      // Read the pin again to confirm the car is off.
      currentStatus.accessoryState = readPin(AccessoryPin);
      if(currentStatus.accessoryState == LOW)
      {
        serialPrintLn("Accessory Power: OFF ");
        // The car has been turned off.
        turnOffAccLed();
        // Keep current status as the old status
        currentStatus.oldLidStatus = currentStatus.lidStatus;  
        if(isLidOpen())
        {        
          // If car is turned off and lid is open, close lid.
          currentStatus.lidStatus = closeLid();
        }
        saveCurrentStatus(currentStatus.oldLidStatus);
         digitalWrite(PowerMOSPin, LOW); //shutdown all!
      }      
    }    
  }  
  currentStatus.oldAccessoryState = currentStatus.accessoryState; // Keep track of accessory state
}
