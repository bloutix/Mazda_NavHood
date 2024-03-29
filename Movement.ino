/// Moves the lid forward.
void moveForward()
{
  digitalWrite(EnaPin, HIGH);
  digitalWrite(BackwardsPin, LOW);
  digitalWrite(ForwardPin, HIGH);
}

/// Moves the lid backwards
void moveBackwards()
{
  digitalWrite(EnaPin, HIGH);
  digitalWrite(ForwardPin, LOW);
  digitalWrite(BackwardsPin, HIGH);
}

/// Stops the motor movement.
void stopMovement()
{
  digitalWrite(EnaPin, HIGH);
  digitalWrite(ForwardPin, LOW);
  digitalWrite(BackwardsPin, LOW);
}

/// Reads the lid position sensor, detecting its location.
int readSensor()
{
    //digitalWrite(SensorRefPin, HIGH); //activate trimpot ref
  return analogRead(SensorPin);
    //digitalWrite(SensorRefPin, LOW); //activate trimpot ref
}

/// Moves the lid to the specified position.
/// minPosition: less than current position limit.
/// maxPosition: greater than current position limit.
/// movementDirection: the direction of the movement, forward or backwards.
/// newLidStatus: the status after the movement.
int moveLid(int minPosition, int maxPosition, int movementDirection, int newLidStatus)
{  
  int currentPosition = readSensor(); // Get current position
  while(minPosition <= currentPosition && currentPosition <= maxPosition)
  {
    if(movementDirection == ForwardMovement)
    {
      serialPrintLn("P: ", currentPosition);
      moveForward();
    } 
    else
    {
      moveBackwards();
    }    
    currentPosition = readSensor();
  }  
  stopMovement();
  return currentPosition;
}

/// Closes the lid if its opened or tilted.
int closeLid()
{
  serialPrintLn("Closing Lid...");
  int newPosition = readSensor();
  while(OpenPositionMin <= newPosition && newPosition <= ClosedPositionMin)
  {
    moveForward();
    newPosition = readSensor();
  }
  stopMovement();   
  serialPrintLn("Complete");
  digitalWrite(EnaPin, LOW);  //disable H bridge when its closed
  return ClosedStatus;
}

/// Opens the lid if its closed
int openLid()
{
  serialPrintLn("Opening Lid...");
  int newPosition = readSensor();
  while(OpenPositionMax <= newPosition && newPosition <= ClosedPositionMax)
  {
    moveBackwards();
    newPosition = readSensor();
  }
  stopMovement();  
  serialPrintLn("Complete");  
  return OpenStatus;
}

// Opens the lid to the specified status.
int openLid(int newLidStatus)
{
  serialPrintLn("Opening Lid to defined Status...");  
  // Gets the destination position for the new specified status
  LidPosition lidPosition = getPositionRangeFromStatus(newLidStatus);
  
  // Move Lid
  moveLid(lidPosition.MaxPosition, ClosedPositionMax, BackwardsMovement, newLidStatus);    
  serialPrintLn("Complete");
  return newLidStatus;
}

/// Tilts the lid to any of the 3 possible positions. 
/// The lid needs to be opened for the tilting to work.
int tiltLid(int currentStatus)
{
  serialPrintLn("Tilting Lid...");
  serialPrintLn("Current Status: ", currentStatus);  
  
  // Get Positions for movement
  LidPosition lidPositions = getNextTiltedStatusPositions(currentStatus);
  
  // Tilt Lid
  int currentPosition = moveLid(lidPositions.MinPosition, lidPositions.MaxPosition, lidPositions.MovementDirection, lidPositions.LidStatus);
  
  serialPrintLn("Current Position: ", currentPosition);
  serialPrintLn("New Status: ", lidPositions.LidStatus);  
  serialPrintLn("Complete");
  return lidPositions.LidStatus;
}
