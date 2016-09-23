#define rightMotor 4
#define rightMotorPWM 5
#define leftMotor 7
#define leftMotorPWM 6

void setup()
{
  pinMode(rightMotor, OUTPUT);
  pinMode(rightMotorPWM, OUTPUT);
  pinMode(leftMotor, OUTPUT);
  pinMode(leftMotorPWM, OUTPUT);
  } 

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


