#include "printf.h"
#include "RF24.h"


//line sensor defines
int sensor[4] = { 1, 2, 3, 4 };
int sensorReadings[4] = { 0 };
int activeSensors = 0;
float totalReading = 0;
float avgReading = 0; // value from 0-9 to indicate position of line between sensor 1 - 4
float lastReading = 0;

int rightMotorSpeed = 0;
int leftMotorSpeed = 0;
int maxMotorSpeed = 140;
float blackThreshold = 0.75;
int on_line = 0;

int pwm_a = 5;
int pwm_b = 6;
int dir_a = 4;
int dir_b = 7;

bool c = false;
int lastWideSensor = 9;

float error = 0;
float previousError = 0;
float totalError = 0;

RF24 myRadio(9, 10);

// Topology
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL }; // Radio pipe addresses for the 2 nodes to communicate.

bool onLine = true;

float power = 0 ; // Process Variable value calculated to adjust speeds and keep on line
float kp = 80;  // This is the Proportional value. Tune this value to affect follow_line performance
float kd = 7;
float ki = 0.0040;

void setup()
{
  Serial.begin(9600);
  printf_begin();

  myRadio.begin();
  myRadio.openWritingPipe(pipes[1]);        // The radios need two pipes to communicate. One reading pipe and one writing pipe.
  myRadio.openReadingPipe(1, pipes[0]);
  myRadio.startListening();                 // Switch to reading pipe

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
  if (readWideSensor(0)) {
    lastWideSensor = 0;
  }
  if (readWideSensor(5)) {
    lastWideSensor = 5;
  }

  onLine = checkWideSensors();

  if (onLine)
  {
    digitalWrite(dir_a, HIGH);
    digitalWrite(dir_b, LOW);
    followLine();
  }
  else {
    turn();

  }
}  // end main loop

bool checkWideSensors() {


  if (readSensor(3) || readSensor(1) || readSensor(2) || readSensor(4) )
  {
    return true;
  }
  else if (readWideSensor(0))  {
    return false;
  }
  else if (readWideSensor(5)) {
    return false;
  }
  else {
    return false;
  }
}

void turn()
{
  if (lastWideSensor == 0)
  {
    digitalWrite(dir_a, LOW);
    digitalWrite(dir_b, LOW);
    analogWrite(pwm_a, maxMotorSpeed);
    analogWrite(pwm_b, maxMotorSpeed);
    // delay(200);

  }
  if (lastWideSensor == 5) {
    digitalWrite(dir_a, HIGH);
    digitalWrite(dir_b, HIGH);
    analogWrite(pwm_a, maxMotorSpeed);
    analogWrite(pwm_b, maxMotorSpeed);
    // delay(200);
  }
}

// line following subroutine
void followLine()
{
  calcPID();
  //Serial.println(rightMotorSpeed);
  analogWrite(pwm_a, leftMotorSpeed);
  analogWrite(pwm_b, rightMotorSpeed);

} // end follow_line


void readLine() {

  for (int i = 0; i < 4; i++) {
    sensorReadings[i] = readSensor(sensor[i]);
    if (sensorReadings[i] == 1) {
      activeSensors += 1;
    }
    totalReading += sensorReadings[i] * (i + 1);
  }

  if (activeSensors == 0) {

    if (readWideSensor(0) || lastWideSensor == 0) {
      avgReading = -1;
      return;
    }
    if (readWideSensor(5) || lastWideSensor == 5) {
      avgReading = 6;
      return;
    }
    /*if (lastReading == 4) {
      avgReading = 6;
      return;
      }
      else {
      avgReading = -1;
      return;
      }*/
  }
  if (activeSensors != 0)
  {
    avgReading = totalReading / activeSensors;
  } else
  {
    avgReading = 0;
  }
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
    case 0: // read sensor 0.
      sensorValue = analogRead(A0);
      break;
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
    case 5: // read sensor 5.
      sensorValue = analogRead(A5);
      break;
  }
  detectedValue = static_cast<float>(sensorValue) / static_cast<float>(range);
  if (detectedValue > blackThreshold) {
    return true;
  }
  else
  {
    return false;
  }
}

bool readWideSensor(int sensor)
{
  int sensorValue;
  int range = 1024;
  float detectedValue;

  switch (sensor)
  {
    case 0: // read sensor 0.
      sensorValue = analogRead(A0);
      break;
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
    case 5: // read sensor 5.
      sensorValue = analogRead(A5);
      break;
  }
  detectedValue = static_cast<float>(sensorValue) / static_cast<float>(range);
  if (detectedValue > 0.85) {
    return true;
  }
  else
  {
    return false;
  }
}

void calcPID() {

  readLine();

  previousError = error;
  error = avgReading - 2.5;
  totalError += error;



  power = (kp * error) + (kd * (error - previousError)) + (ki * totalError);


  Serial.print(previousError);
  Serial.print(" ");
  Serial.print(error);
  Serial.print(" ");
  Serial.println(power);

  if ( power > maxMotorSpeed) {
    power = maxMotorSpeed;
  }
  if ( power < -maxMotorSpeed ) {
    power = -maxMotorSpeed;
  }

  if (power < 0) //Turn Left
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
  //  if (messageId != lastMessageId) {
  myRadio.stopListening();
  unsigned long id = messageId;
  bool ok = myRadio.write(&id, sizeof(unsigned long));
  if (ok) {
    printf("Message Send:  %d \n", messageId);
  }
  lastMessageId = messageId;
  myRadio.startListening();

}



