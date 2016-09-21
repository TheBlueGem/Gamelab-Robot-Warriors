#include <QTRSensors.h>
#include <SPI.h>
#include "RF24.h"
#include "Tile.h"
#include "printf.h"
#define rightMaxSpeed 150 // max speed of the robot
#define leftMaxSpeed 150 // max speed of the robot
#define rightBaseSpeed 100 // this is the speed at which the motors should spin when the robot is perfectly on the line
#define leftBaseSpeed 100 

#define NUM_SENSORS   4    // number of sensors used
#define NUM_SAMPLES_PER_SENSOR  4  // average 4 analog samples per sensor reading
#define TIMEOUT       2500  // waits for 2500 microseconds for sensor outputs to go low
#define EMITTER_PIN   QTR_NO_EMITTER_PIN  // emitter control pin not used.  If added, replace QTR_NO_EMITTER_PIN with pin#

QTRSensorsAnalog qtrrc((unsigned char[]) { 1, 2, 3, 4
}, NUM_SENSORS  );
unsigned int sensorValues[NUM_SENSORS]; // array with individual sensor reading values
unsigned int line_position = 0; // value from 0-7000 to indicate position of line between sensor 0 - 7

// motor tuning vars
int calSpeed = 145;   // tune value motors will run while auto calibration sweeping turn over line (0-255)

// Proportional Control loop vars
float error = 0;
float previousError = 0;
float totalError = 0;
float PV = 0 ; // Process Variable value calculated to adjust speeds and keep on line
float kp = 0.1;  // This is the Proportional value. A higher proportional value makes the robot steer more aggressively to keep the line under him. 
float kd = 0.05; // This is the Derivative value. It is used to keep the robot from overshooting the line when he's adjusting to keep the line under him. 
                // A higher derivative value will make the robot countersteer earlier.
float ki = 0.18; // This is the Integral value. It is used to reduce the systematic errors over time. This way the robot will attempt to overshoot 
                // the line less the longer he's active. A higher integral value will stabilize the robot faster.
int m1Speed = 0; // (Left motor)
int m2Speed = 0; // (Right motor)


// digital pins used: 2 = rf; 4,5,6,7 = motorshield; 3,11,12,13 = radio
// analog pins used: 0 = sensor 1; 1 = sensor 2; 2 = sensor 3; 3 = sensor 4; 4 = sensor 5;

Tile* tiles[50];
int robotOrientation[50];

int amountOfTiles = 0;
int amountOfPackages = 0;

RF24 myRadio(9, 10);

// Topology
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL }; // Radio pipe addresses for the 2 nodes to communicate.

struct package
{
  int id;
  int tile;
  int tileOrientation;
  int xPosition;
  int yPosition;
  int robotOrientation;
};

typedef struct package Package;

int orientation = 0; // 0 = north, 1 = east, 2 = south, 3 = west
int xPosition = 0;
int yPosition = 0;

//int followLineDifference = 3;
int blackThreshold = 60;

int leftSensorWhiteValue;
int rightSensorWhiteValue;
boolean logging = true;

int junction = 0; //followLine = 0, left = 1, straight = 2, right = 3, reverse = 4

// pwm_a/b sets speed.  Value range is 0-255.  For example, if you set the speed at 100 then 100/255 = 39% duty cycle = slow
// dir_a/b sets direction.  LOW is Forward, HIGH is Reverse
int pwm_a = 5;  //PWM control for Ardumoto outputs A1 and A2 is on digital pin 10  (Left motor)
int pwm_b = 6;  //PWM control for Ardumoto outputs B3 and B4 is on digital pin 11  (Right motor)
int dir_a = 4;  //direction control for Ardumoto outputs A1 and A2 is on digital pin 12  (Left motor)
int dir_b = 7;  //direction control for Ardumoto outputs B3 and B4 is on digital pin 13  (Right motor)


void setup()
{
  for (int i = 4; i <= 7; i++) //For Arduino Motor Shield
    pinMode(i, OUTPUT);  //set pin 4,5,6,7 to output mode

  Serial.begin(9600);
  printf_begin();

  pinMode(pwm_a, OUTPUT);
  pinMode(pwm_b, OUTPUT);
  pinMode(dir_a, OUTPUT);
  pinMode(dir_b, OUTPUT);

  myRadio.begin();
  myRadio.openWritingPipe(pipes[1]);        // The radios need two pipes to communicate. One reading pipe and one writing pipe.
  myRadio.openReadingPipe(1, pipes[0]);
  myRadio.startListening();                 // Switch to reading pipe


  // delay to allow you to set the robot on the line, turn on the power,
  // then move your hand away before it begins moving.
  delay(2000);

   // calibrate line sensor; determines min/max range of sensed values for the current course
  for (int i = 0; i <= 100; i++)  // begin calibration cycle to last about 2.5 seconds (100*25ms/call)
  {
    // auto calibration sweeping left/right, tune 'calSpeed' motor speed at declaration
    // just high enough all sensors are passed over the line. Not too fast.
    /* if (i == 0 || i == 60) // slow sweeping turn right to pass sensors over line
      {
       digitalWrite(dir_a, LOW);
       analogWrite(pwm_a, calSpeed);
       digitalWrite(dir_b, LOW);
       analogWrite(pwm_b, calSpeed);
      }

      else if (i == 20 || i == 100) // slow sweeping turn left to pass sensors over line
      {
       digitalWrite(dir_a, HIGH);
       analogWrite(pwm_a, calSpeed);
       digitalWrite(dir_b, HIGH);
       analogWrite(pwm_b, calSpeed);
    */

    qtrrc.calibrate(); // reads all sensors with the define set 2500 microseconds (25 milliseconds) for sensor outputs to go low.
    //printf("Left sensor value: %d Mid sensor value: %d Right sensor value %d \n", sensorValues[0], sensorValues[1], sensorValues[2]);
  }  // end calibration cycle

  line_position = qtrrc.readLine(sensorValues);

  // find near center
  while (line_position > 1650) // continue loop until line position is near center
  {
    line_position = qtrrc.readLine(sensorValues);
    Serial.println("Line is at center");
  }

  // stop both motors
  analogWrite(pwm_b, 0); // stop right motor first which kinda helps avoid over run
  analogWrite(pwm_a, 0);

  // delay as indicator setup and calibration is complete
  delay(1000);

  Serial.println("Setup Done"); 
}

int readSensor(int sensor)
{ //returns a percentage of how black the sensor is. 0 = white; 100 = black.
  double result;
  int detectedValue;
  int range = 1024;
  int sensorValue;
  switch (sensor)
  {
  case 0: // read sensor 0.
    sensorValue = analogRead(A0);   
    detectedValue = sensorValue;
    break;
  case 5: // read sensor 5.
    sensorValue = analogRead(A5);   
    detectedValue = sensorValue;
    break;
  }

  result = ((double)detectedValue / (double)range) * 100.0;

  return (int)result;
}

void setNewLocation()
{
  switch (orientation)
  {
  case 0:
    yPosition++;
    break;
  case 1:
    xPosition++;
    break;
  case 2:
    yPosition--;
    break;
  case 3:
    xPosition--;
    break;
  }
}

void moveStraight()
{ 
    // Motor1(255, false);
     //  Motor2(255, true);
}

void moveLeft()
{
  
    //   Motor1(255, true);
     //  Motor2(255, true);
    //sendLogMessage(9);  
}

void moveWideLeft()
{ 
    //   Motor1(0, true);
    //   Motor2(255, true);
    //sendLogMessage(11); 
}

void moveRight()
{ 
    //   Motor1(255, false);
    //   Motor2(255, false);
    //sendLogMessage(10);
}

void moveWideRight()
{
    //   Motor1(255, false);
    //   Motor2(0, false);
   // sendLogMessage(12); 
}

void turnOff()
{
  //Motor1(0, false);
  //Motor2(0, true);
  if (logging)
  {
    Serial.println("Engines Off");
  }

}

/*void addTile(int tileType, int tileOrientation, int x, int y, int robotOrientation)
{
  Tile* newTile = new Tile(tileType, tileOrientation, x, y);
  //sendMessage(newTile->getType(), newTile->getOrientation(), x, y, orientation);
  boolean tileKnown = false;
  for(int i = 0; i < amountOfTiles; i++)
  {
  if(x == tiles[i]->getXCoordinate() && y == tiles[i]->getYCoordinate())
  { //check if there is a tile on x and y coordinates.
    newTile = tiles[i];
    tileKnown = true;
  }
  }
  if(!tileKnown)
  {
  for(int i = 0; i < amountOfTiles; i++)
  {
    Tile* checkingTile = tiles[i];
    int x2 = checkingTile->getXCoordinate();
    int y2 = checkingTile->getYCoordinate();
    if(x == x2 && y + 1 == y2)
    { //CheckingTile is north of newTile
    newTile->setNorth(checkingTile);
    checkingTile->setSouth(newTile);
    }
    if(x + 1 == x2 && y == y2)
    { //CheckingTile is east of newTile
    newTile->setEast(checkingTile);
    checkingTile->setWest(newTile);
    }
    if(x == x2 && y - 1 == y2)
    { //CheckingTile is south of newTile
    newTile->setSouth(checkingTile);
    checkingTile->setNorth(newTile);
    }
    if(x - 1 == x2 && y == y2)
    { //CheckingTile is west of newTile
    newTile->setWest(checkingTile);
    checkingTile->setEast(newTile);
    }
  }

  bool north = checkDirectionPossibility(tileType, tileOrientation, 0);
  bool east = checkDirectionPossibility(tileType, tileOrientation, 1);
  bool south = checkDirectionPossibility(tileType, tileOrientation, 2);
  bool west = checkDirectionPossibility(tileType, tileOrientation, 3);

  newTile->setOptionNorth(north);
  newTile->setOptionSouth(south);
  newTile->setOptionEast(east);
  newTile->setOptionWest(west);

  //sendMessage(packageId, x, y, (String) ammountOfTiles);
  }

  //tiles[amountOfTiles] = newTile;
  //robotOrientation[amountOfTiles] = rOrientation;
  //amountOfTiles++;
}*/

void detectTile()
{
  int left = readSensor(5);
  int right = readSensor(0);

  printf("Left Value: %d", left);;
  printf(" Right Value: %d\n", right);
  printf("Line Position: %d\n", line_position);

  if(left > blackThreshold)
    {
    if(line_position > 1000 && line_position < 2000)
    {
      if(right > blackThreshold)
      { //bbb
      readTile(0, 0);
      }
      else
      { //bbw
      readTile(1, 1);
      }
    }
    else
    {
      if(right > blackThreshold)
      { //bwb
      readTile(1, 2);
      }
      else
      { //bww
      readTile(2, 4);
      }
    }
    }
    else
    {
    if(line_position > 1000 && line_position < 2000)
    {
      if(right > blackThreshold)
      { //wbb
      readTile(1, 3);
      }
      else
      { //wbw
      readTile(3, 6);      
     }
    }
    else
    {
      if(right > blackThreshold)
      { //wwb
      readTile(2, 5);
      }
      else
      { //www
      detectEnd();
      }
    }
  }
}

/**
* Read what kind of Tile we have encountered
*/
void readTile(int type, int caseId)
{
  int arrivalDirection = -1;

  switch (caseId)
  {
  case 0: // Arrived at intersection.
    if (logging)
    {
      sendLogMessage(0);
    }
    arrivalDirection = orientation + 1;
    //addTile(1, orientation + 1, xPosition, yPosition, orientation);
    moveWideLeft();
    break;
  case 1: // Arrived from the right side of a T-tile.
    if (logging)
    {
      sendLogMessage(1);
    }
    arrivalDirection = orientation;
    moveWideRight();
    //addTile(1, orientation, xPosition, yPosition, orientation);    
    break;

  case 2: // Arrived from the bottom side of a T-tile.
    if (logging)
    {
      sendLogMessage(2);
    }
    moveWideLeft();
    //addTile(2, orientation + 1, xPosition, yPosition, orientation);
    arrivalDirection = orientation + 1;
    break;

  case 3: // Arrived from the left side of a T-tile.
    if (logging)
    {
      sendLogMessage(3);
    }
    //addTile(2, orientation, xPosition, yPosition, orientation);
    arrivalDirection = orientation;
    moveWideRight();
    break;

  case 4: // Arrived from the right side of a corner.
    if (logging)
    {
      sendLogMessage(4);
    }
    //addTile(2, orientation + 2, xPosition, yPosition, orientation);
    arrivalDirection = orientation + 2;
    moveWideLeft();
    break;

  case 5: // Arrived from the left side of a corner.
    if (logging)
    {
      sendLogMessage(5);
    }
    //addTile(3, 0, xPosition, yPosition, orientation);
    arrivalDirection = 0;
    moveWideLeft();
    break;

  case 6: // Going over a straight tile
    if (logging)
    {
      sendLogMessage(8);
    }
        
    follow_line(line_position); 
    break;
  }

  //makeMove();
}

int startTime = millis();
int elapsedTime = 0;

void detectEnd()
{
  //moveStraight();
  if (elapsedTime < 1000)
  {
    elapsedTime = millis() - startTime;
  }
  else
  {
    int left = readSensor(1);
    int middle = readSensor(2);
    int right = readSensor(3);
    if (left > blackThreshold && middle > blackThreshold && right > blackThreshold)
    {
      if (logging)
      {
        sendLogMessage(6);
      }
      //addTile(4, orientation, xPosition, yPosition, orientation);
      //sendMessage(4, orientation, xPosition, yPosition, orientation);
    }
    else
    {
      if (logging)
      {
        sendLogMessage(7);
      }
      //addTile(0, orientation, xPosition, yPosition, orientation);
      //sendMessage(0, orientation, xPosition, yPosition, orientation);
      //makeMove();
    }
    elapsedTime = 0;
  }
}

int lastMessageId = -1;

void logString(int messageId, String message)
{
  if (messageId != lastMessageId) {
    Serial.println(message);
    lastMessageId = messageId;
  }
}

int currentPackageId = 0;

void sendLogMessage(int messageId) {
  if (messageId != lastMessageId) {
    myRadio.stopListening();
    unsigned long id = messageId;
    bool ok = myRadio.write(&id, sizeof(unsigned long));
    if (ok) {
      printf("Message Send:  %d \n", messageId);
    }
    lastMessageId = messageId;
  }

}
void sendMessage(int tile, int tileOrientation, int x, int y, int robotOrientation)
{
  //bool received = false;
  // First, stop listening so we can talk.
  Package data = { currentPackageId, tile, tileOrientation, x, y, robotOrientation };
  //receiveMessage();
  myRadio.stopListening();
  if (!myRadio.write(&data, sizeof(data)))
  {
    Serial.println("failed.");
  }

  myRadio.openWritingPipe(pipes[1]);
  myRadio.openReadingPipe(1, pipes[0]);
  //myRadio.startListening();
  //receiveMessage();
}

void receiveMessage()
{

  Serial.println("startListening");
  Package data;
  while (myRadio.available())
  {
    Serial.println("readPackage");
    myRadio.read(&data, sizeof(data));
    if (data.tile >= 0)
    {
      Serial.println("addTile");
      data.id = -1;
      //myRadio.writeAckPayload(pipes[1],&data, sizeof(data) );
      Serial.print("R: ");
      Serial.print("id: ");
      Serial.print(data.id);
      Serial.print("Tile: ");
      Serial.print(data.tile);
      Serial.print(", tOrientation: ");
      Serial.print(data.tileOrientation);
      Serial.print(", x: ");
      Serial.print(data.xPosition);
      Serial.print(", y: ");
      Serial.print(data.yPosition);
      Serial.print(", rOrientation: ");
      Serial.println(data.robotOrientation);
      //addTile(data.tile, data.tileOrientation, data.xPosition, data.yPosition, data.robotOrientation);
    }
  }

}

//make a choice based on the orientation of the robot, tile type and tile orientation. Always try to go right, if not go straight, if not go left or turn around.
void makeMove()
{
  //get current tile
  Tile* currentTile;
  for (int i = 0; i < amountOfTiles; i++)
  {
    if (xPosition == tiles[i]->getXCoordinate() && yPosition == tiles[i]->getYCoordinate()) //check if there is a tile on the current x and y coordinates.
    {
      currentTile = tiles[i];
    }
  }

  //check if robot has been on this tile with the same orientation
  bool sameOrientationAndTile = false;
  for (int j = 0; j < amountOfTiles; j++)
  {
    Tile* t = tiles[j];
    int robotOrientationOnTile = robotOrientation[j];
    //    TilePackage tp = tilePackages[j];
    if (t->getXCoordinate() == currentTile->getXCoordinate() && t->getYCoordinate() == currentTile->getYCoordinate() && robotOrientationOnTile == orientation)
    {
      sameOrientationAndTile = true;
    }
  }

  switch (orientation)
  {
  case 0: //orientation = North
    if (sameOrientationAndTile)
    {
      if (currentTile->getOptionEast())
      {
        junction = 3;
      }
      else if (currentTile->getOptionNorth())
      {
        junction = 2;
      }
      else if (currentTile->getOptionWest())
      {
        junction = 1;
      }
      else
      {
        junction = 4;
      }
    }
    else
    {
      if (currentTile->getOptionEast())
      {
        if (currentTile->getOptionNorth())
        {
          junction = 2;
        }
        else if (currentTile->getOptionWest())
        {
          junction = 1;
        }
        else
        {
          junction = 3;
        }
      }
      else if (currentTile->getOptionNorth())
      {
        if (currentTile->getOptionWest())
        {
          junction = 1;
        }
        else
        {
          junction = 2;
        }
      }
      else if (currentTile->getOptionWest())
      {
        junction = 1;
      }
      else
      {
        junction = 4;
      }
    }
    /*else if(sameOrientationAndTile == 2)
    {
    if(currentTile->getOptionEast())
    {
    if(currentTile->getOptionNorth())
    {
    if(currentTile->getOptionWest())
    {
    junction = 1;
    }
    else
    {
    junction = 3;
    }
    }
    else
    {
    junction = 3;
    }
    }
    else if(currentTile->getOptionNorth())
    {
    junction = 2;
    }
    else if(currentTile->getOptionWest())
    {
    junction = 1;
    }
    else
    {
    junction = 4;
    }
    }*/
    break;
  case 1: //orientation = East
    if (sameOrientationAndTile)
    {
      if (currentTile->getOptionSouth())
      {
        junction = 3;
      }
      else if (currentTile->getOptionEast())
      {
        junction = 2;
      }
      else if (currentTile->getOptionNorth())
      {
        junction = 1;
      }
      else
      {
        junction = 4;
      }
    }
    else
    {
      if (currentTile->getOptionSouth())
      {
        if (currentTile->getOptionEast())
        {
          junction = 2;
        }
        else if (currentTile->getOptionNorth())
        {
          junction = 1;
        }
        else
        {
          junction = 3;
        }
      }
      else if (currentTile->getOptionEast())
      {
        if (currentTile->getOptionNorth())
        {
          junction = 1;
        }
        else
        {
          junction = 2;
        }
      }
      else if (currentTile->getOptionNorth())
      {
        junction = 1;
      }
      else
      {
        junction = 4;
      }
    }
    /*else if(sameOrientationAndTile == 2)
    {
    if(currentTile->getOptionSouth())
    {
    if(currentTile->getOptionEast())
    {
    if(currentTile->getOptionNorth())
    {
    junction = 1;
    }
    else
    {
    junction = 3;
    }
    }
    else
    {
    junction = 3;
    }
    }
    else if(currentTile->getOptionEast())
    {
    junction = 2;
    }
    else if(currentTile->getOptionNorth())
    {
    junction = 1;
    }
    else
    {
    junction = 4;
    }
    }*/
    break;
  case 2:
    if (sameOrientationAndTile)
    {
      if (currentTile->getOptionWest())
      {
        junction = 3;
      }
      else if (currentTile->getOptionSouth())
      {
        junction = 2;
      }
      else if (currentTile->getOptionEast())
      {
        junction = 1;
      }
      else
      {
        junction = 4;
      }
    }
    else
    {
      if (currentTile->getOptionWest())
      {
        if (currentTile->getOptionSouth())
        {
          junction = 2;
        }
        else if (currentTile->getOptionEast())
        {
          junction = 1;
        }
        else
        {
          junction = 3;
        }
      }
      /*else if(currentTile->getOptionSouth())
      {
      if(currentTile->getOptionEast())
      {
      junction = 1;
      }
      else
      {
      junction = 2;
      }
      }
      else if(currentTile->getOptionEast())
      {
      junction = 1;
      }
      else
      {
      junction = 4;
      }
      }
      else if(sameOrientationAndTile == 2)
      {
      if(currentTile->getOptionWest())
      {
      if(currentTile->getOptionSouth())
      {
      if(currentTile->getOptionEast())
      {
      junction = 1;
      }
      else
      {
      junction = 3;
      }
      }
      else
      {
      junction = 3;
      }
      }
      else if(currentTile->getOptionSouth())
      {
      junction = 2;
      }
      else if(currentTile->getOptionEast())
      {
      junction = 1;
      }
      else
      {
      junction = 4;
      }
      }*/
      break;
  case 3:
    if (sameOrientationAndTile)
    {
      if (currentTile->getOptionNorth())
      {
        junction = 3;
      }
      else if (currentTile->getOptionWest())
      {
        junction = 2;
      }
      else if (currentTile->getOptionSouth())
      {
        junction = 1;
      }
      else
      {
        junction = 4;
      }
    }
    else
    {
      if (currentTile->getOptionNorth())
      {
        if (currentTile->getOptionWest())
        {
          junction = 2;
        }
        else if (currentTile->getOptionSouth())
        {
          junction = 1;
        }
        else
        {
          junction = 3;
        }
      }
      else if (currentTile->getOptionWest())
      {
        if (currentTile->getOptionSouth())
        {
          junction = 1;
        }
        else
        {
          junction = 2;
        }
      }
      else if (currentTile->getOptionSouth())
      {
        junction = 1;
      }
      else
      {
        junction = 4;
      }
    }
    /*else if(sameOrientationAndTile == 2)
    {
    if(currentTile->getOptionNorth())
    {
    if(currentTile->getOptionWest())
    {
    if(currentTile->getOptionSouth())
    {
    junction = 1;
    }
    else
    {
    junction = 3;
    }
    }
    else
    {
    junction = 3;
    }
    }
    else if(currentTile->getOptionWest())
    {
    junction = 2;
    }
    else if(currentTile->getOptionSouth())
    {
    junction = 1;
    }
    else
    {
    junction = 4;
    }
    }*/
    break;
    }
  }
}

//check for possible direction based on the type of tile, orientation of the tile and the orientation of the robot.
/*boolean checkDirectionPossibility(int tileType, int tileOrientation, int moveDirection)
{
  if(tileOrientation > 3)
  {
  tileOrientation = tileOrientation - 4;
  }
  switch(moveDirection)
  {
  case 0: //north
  if((tileType == 0 && tileOrientation == 2) || (tileType == 2 && tileOrientation != 1) || (tileType == 1 && (tileOrientation != 0 && tileOrientation != 1)) || tileType == 3)
  {
    return true;
  }
  break;
  case 1: //east
  if((tileType == 0 && tileOrientation == 3) || (tileType == 2 && tileOrientation != 2) || (tileType == 1 && (tileOrientation != 1 && tileOrientation != 2)) || tileType == 3)
  {
    return true;
  }
  break;
  case 2: //south
  if((tileType == 0 && tileOrientation == 0) || (tileType == 2 && tileOrientation != 3) || (tileType == 1 && (tileOrientation != 2 && tileOrientation != 3)) || tileType == 3)
  {
    return true;
  }
  break;
  case 3: //west
  if((tileType == 0 && tileOrientation == 1) || (tileType == 2 && tileOrientation != 0) || (tileType == 1 && (tileOrientation != 0 && tileOrientation != 3)) || tileType == 3)
  {
    return true;
  }
  break;
  }
  return false;
}*/

void follow_line(int line_position) //follow the line
{  
  int error = line_position - 1500;

  int motorSpeed = kp * error + kd * (error - previousError) + ki * totalError;
  previousError = error;

  int rightMotorSpeed = rightBaseSpeed + motorSpeed;
  int leftMotorSpeed = leftBaseSpeed - motorSpeed;
  
  if (rightMotorSpeed > rightMaxSpeed ) rightMotorSpeed = rightMaxSpeed; // prevent the motor from going beyond max speed
  if (leftMotorSpeed > leftMaxSpeed ) leftMotorSpeed = leftMaxSpeed; // prevent the motor from going beyond max speed
  if (rightMotorSpeed < 0) rightMotorSpeed = 0; // keep the motor speed positive
  if (leftMotorSpeed < 0) leftMotorSpeed = 0; // keep the motor speed positive


      digitalWrite(dir_a, HIGH);
      analogWrite(pwm_a, rightMotorSpeed);
      digitalWrite(dir_b, LOW);
      analogWrite(pwm_b, leftMotorSpeed);

      //Serial.println("Going straight");
       
     
} // end follow_line

void loop()
{ 
  //printf("Values left: %d middle: %d right: %d\n", left, middle, right);
   line_position = qtrrc.readLine(sensorValues);
   
   detectTile();
}




















