#include <QTRSensors.h>

#define Kp 0 // experiment to determine this, start by something small that just makes your bot follow the line at a slow speed
#define Kd 0 // experiment to determine this, slowly increase the speeds and adjust this value. ( Note: Kp < Kd) 
#define rightMaxSpeed 255 // max speed of the robot
#define leftMaxSpeed 255 // max speed of the robot
#define rightBaseSpeed 230 // this is the speed at which the motors should spin when the robot is perfectly on the line
#define leftBaseSpeed 230  // this is the speed at which the motors should spin when the robot is perfectly on the line
#define NUM_SENSORS  5     // number of sensors used
#define TIMEOUT      2500  // waits for 2500 us for sensor outputs to go low

#define rightMotor 4
#define rightMotorPWM 5
#define leftMotor 7
#define leftMotorPWM 6

QTRSensorsRC qtrrc((unsigned char[]) { 14, 15, 16, 17, 18 }, NUM_SENSORS, TIMEOUT, QTR_NO_EMITTER_PIN); // sensor connected through analog pins A0 - A5 i.e. digital pins 14-19

unsigned int sensorValues[NUM_SENSORS];

void setup()
{
  pinMode(rightMotor, OUTPUT);
  pinMode(rightMotorPWM, OUTPUT);
  pinMode(leftMotor, OUTPUT);
  pinMode(leftMotorPWM, OUTPUT);
  } 

int lastError = 0;

void loop()
{
  digitalWrite(rightMotor, HIGH);
  analogWrite(rightMotorPWM, 100);
  digitalWrite(leftMotor, LOW);
  analogWrite(leftMotorPWM, 100);
}

void turn_left()
{
  digitalWrite(rightMotor, HIGH);
  analogWrite(rightMotorPWM, 255);
  digitalWrite(leftMotor, LOW);
  analogWrite(leftMotorPWM, 255);
}


void turn_right()
{
  digitalWrite(rightMotor, LOW);
  analogWrite(rightMotorPWM, 255);
  digitalWrite(leftMotor, HIGH);
  analogWrite(leftMotorPWM, 255);
}


