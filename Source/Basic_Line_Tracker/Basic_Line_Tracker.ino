#include <SPI.h>

//int followLineDifference = 3;
int blackThreshold = 60;

int leftSensorWhiteValue;
int middleSensorWhiteValue;
int rightSensorWhiteValue;
boolean logging = true;
int movementMode; // 0 = engines off, 1 = straight, 2 = reverse, 3 = left, 4 = wideLeft, 5 = right, 6 = wideRight
int junction = 0; //followLine = 0, left = 1, straight = 2, right = 3, reverse = 4


void Motor1(int pwm, boolean reverse)
{
  analogWrite(6, pwm); //set pwm control, 0 for stop, and 255 for maximum speed
  if(reverse)
  {
    digitalWrite(7, HIGH);
  }
  else
  {
    digitalWrite(7, LOW);
  }
}

void Motor2(int pwm, boolean reverse)
{
  analogWrite(5, pwm);
  if(reverse)
  {
    digitalWrite(4, HIGH);
  }
  else
  {
    digitalWrite(4, LOW);
  }
}


void setup()
{
  for(int i = 4; i <= 7; i++) //For Arduino Motor Shield
    pinMode(i, OUTPUT);  //set pin 4,5,6,7 to output mode

  int leftSensor = 0;
  int middleSensor = 0;
  int rightSensor = 0;

  Serial.begin(57600);


  for(int i = 0; i < 10; i++)
  {
    leftSensor = analogRead(A0);
    middleSensor = analogRead(A1);
    rightSensor = analogRead(A2);

    leftSensorWhiteValue += leftSensor;
    middleSensorWhiteValue += middleSensor;
    rightSensorWhiteValue += rightSensor;
  }

  leftSensorWhiteValue = leftSensorWhiteValue / 10;
  middleSensorWhiteValue = middleSensorWhiteValue / 10;
  rightSensorWhiteValue = rightSensorWhiteValue / 10;
  
  Motor1(0, false);
  Motor2(0, true);
  Serial.println("Setup");
  findLine();
}

void findLine()
{
  Serial.println("Finding line....");
  unsigned long startTime = millis();
  unsigned long searchTime = 0;


  int left = 0;
  int middle = 0;
  int right = 0;
  while(left < blackThreshold)
  {
    left = readSensor(1);
    middle = readSensor(2);
    right = readSensor(3);    
    
    if(searchTime > 5000 && searchTime < 6000)
    {
      moveStraight();
      searchTime =  millis() - startTime;
    }
    else if(searchTime > 6000)
    {
      startTime = millis();
      searchTime = 0;
    }
    else
    {
      moveWideLeft();
      searchTime = millis() - startTime;
    }    
  }

  Serial.println("Line found!");
}

int readSensor(int sensor)
{ //returns a percentage of how black the sensor is. 0 = white; 100 = black.
  double result;
  int detectedValue;
  int range;
  int sensorValue;
  switch(sensor)
  {
  case 1: // read sensor 1.
    sensorValue = analogRead(A0);    
    if(sensorValue < leftSensorWhiteValue)
    {
      leftSensorWhiteValue = sensorValue;
    }
    detectedValue = sensorValue - leftSensorWhiteValue;
    range = 1024 - leftSensorWhiteValue;
    break;
  case 2: // read sensor 2.
    sensorValue = analogRead(A1);
    if(sensorValue < middleSensorWhiteValue)
    {
      middleSensorWhiteValue = sensorValue;
    }
    detectedValue = sensorValue - middleSensorWhiteValue;
    range = 1024 - middleSensorWhiteValue;
    break;
  case 3: // read sensor 3.
    sensorValue = analogRead(A2);
    if(sensorValue < rightSensorWhiteValue)
    {
      rightSensorWhiteValue = sensorValue;
    }
    detectedValue = sensorValue - rightSensorWhiteValue;
    range = 1024 - rightSensorWhiteValue;
    break;
  }

    result = ((double)detectedValue / (double)range) * 100.0;
  
  return (int)result;
}

void moveStraight()
{
  if(movementMode != 1)
  {
    Motor1(255, false);
    Motor2(255, true);
    movementMode = 1;
  }
}

void moveLeft()
{
  if(movementMode != 3)
  {
    Motor1(255, true);
    Motor2(255, true);
    movementMode = 3;
  }
}

void moveWideLeft()
{
  if(movementMode != 4)
  {
    Motor1(0, true);
    Motor2(255, true);
    movementMode = 3;
  }
}

void moveRight()
{
  if(movementMode != 5)
  {
    Motor1(255, false);
    Motor2(255, false);
    movementMode = 4;
  }
}

void moveWideRight()
{
  if(movementMode != 6)
  {
    Motor1(255, false);
    Motor2(0, false);
    movementMode = 4;
  }
}

void turnOff()
{
  Motor1(0, false);
  Motor2(0, true);
  if(logging && movementMode != 0)
  {
    Serial.println("Engines Off");
  }

  movementMode = 0;
}

void loop()
{    
  int left = readSensor(1);
  int middle = readSensor(2);
  int right = readSensor(3);

  if(middle > blackThreshold && left < blackThreshold && right < blackThreshold){
    moveStraight();
  }
  else if(right > blackThreshold)
  {
    moveWideRight();
  }
  else
  {
    moveWideLeft(); 
  }
}












