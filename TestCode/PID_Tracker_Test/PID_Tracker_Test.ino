#include "printf.h"

//line sensor defines
int sensor[4] = { 1,2,3,4 };
int sensorReadings[4] = { 0 };
int activeSensors = 0;
float totalReading = 0;
float avgReading = 0; // value from 0-9 to indicate position of line between sensor 1 - 4
float lastReading = 0;

int rightMotorSpeed = 0;
int leftMotorSpeed = 0;
int maxMotorSpeed = 140;
float blackThreshold = 0.6;
int on_line = 0;

int pwm_a = 5; 
int pwm_b = 6; 
int dir_a = 4; 
int dir_b = 7; 

float error = 0;
float previousError = 0;
float totalError = 0;

float power = 0 ; // Process Variable value calculated to adjust speeds and keep on line
float kp = 80;  // This is the Proportional value. Tune this value to affect follow_line performance
float kd = 7;
float ki = 0.40;

void setup()
{
  Serial.begin(9600);
  printf_begin();
  //Set control pins to be outputs
  pinMode(pwm_a, OUTPUT);
  pinMode(pwm_b, OUTPUT);
  pinMode(dir_a, OUTPUT);
  pinMode(dir_b, OUTPUT); 
  Serial.println("Setup Done");
  digitalWrite(dir_a, HIGH);
  digitalWrite(dir_b, LOW);
} // end setup

void loop()
{  
  followLine();
}  // end main loop

// line following subroutine
void followLine()
{  
  calcPID();
  //Serial.println(rightMotorSpeed);
  analogWrite(pwm_a, leftMotorSpeed);
  analogWrite(pwm_b, rightMotorSpeed);
} // end follow_line


void readLine(){
  for(int i = 0; i < 4; i++){   
    sensorReadings[i] = readSensor(sensor[i]);
    if(sensorReadings[i]==1) { activeSensors += 1; }
    totalReading += sensorReadings[i] * (i+1);    
  }

  if(activeSensors == 0){
    if(lastReading == 4){
    avgReading = 6;
    return;
    }
    else{
      avgReading = -1;
      return;        
    }
  }
  avgReading = totalReading / activeSensors;  
  lastReading = avgReading;
  totalReading = 0; activeSensors = 0;
}

bool readSensor(int sensor)
{ 
  int sensorValue;
  int range = 1024;
  float detectedValue;
  
  switch (sensor)
  {
    case 1: // read sensor 1.
      sensorValue = analogRead(A1);           
      break;
    case 2: // read sensor 2.
      sensorValue = analogRead(A2);  
      break;
    case 3: // read sensor 3.
      sensorValue = analogRead(A3);  
      break;
    case 4: // read sensor 4.
      sensorValue = analogRead(A4);  
      break;  
  }
  detectedValue = static_cast<float>(sensorValue) / static_cast<float>(range);  
  if(detectedValue > blackThreshold){
    return true;
  }
  else
  {
    return false;
  }
}

void calcPID(){
  readLine();

  previousError = error;
  error = avgReading - 2.5;
  totalError += error;

  power = (kp * error);
  
  //+ (kd * (error - previousError)) + (ki * totalError)
  
  Serial.print(previousError);
  Serial.print(" ");
  Serial.print(error);
  Serial.print(" ");
  Serial.println(power);
  if( power > maxMotorSpeed) { power = maxMotorSpeed; }
  if( power < -maxMotorSpeed ) { power = -maxMotorSpeed; }

  if(power < 0) //Turn Left
  {
    rightMotorSpeed = maxMotorSpeed;
    leftMotorSpeed = maxMotorSpeed - abs(power);
  }
  else
  {
    rightMotorSpeed = maxMotorSpeed - power;
    leftMotorSpeed = maxMotorSpeed;
  }  
}





