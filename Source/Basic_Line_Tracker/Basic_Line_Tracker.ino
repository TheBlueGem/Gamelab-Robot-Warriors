#include <QTRSensors.h>

#include <SPI.h>

 
// create an object for your type of sensor (RC or Analog)
// in this example we have three sensors on analog inputs 0 - 2, a.k.a. digital pins 14 - 16
QTRSensorsRC qtr((char[]) {15, 16, 17}, 3);
// QTRSensorsA qtr((char[]) {1, 2, 3}, 3);
 
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
    // optional: wait for some input from the user, such as  a button press
 
  // then start calibration phase and move the sensors over both
  // reflectance extremes they will encounter in your application:
  int i;
  for (i = 0; i < 250; i++)  // make the calibration take about 5 seconds
  {
    qtr.calibrate();
    delay(20);
  }
 
  // optional: signal that the calibration phase is now over and wait for further
  // input from the user, such as a button press
   unsigned int sensors[3];
   int position = qtr.readLine(sensors);
  Serial.println(sensors.read());
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












