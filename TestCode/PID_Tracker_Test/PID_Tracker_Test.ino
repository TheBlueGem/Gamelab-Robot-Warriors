#include <MsTimer2.h>
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
float blackThreshold = 0.80;
int on_line = 0;

bool timerStarted;

int pwm_a = 5;
int pwm_b = 6;
int dir_a = 4;
int dir_b = 7;

int direction = 0; // 0 = Robot North, 1 = Robot East, 2 = Robot South, 3 = Robot West.

bool c = false;
int lastWideSensor = 9;

float error = 0;
float previousError = 0;
float totalError = 0;

RF24 myRadio(9, 10);

// Topology
const uint64_t pipes[4] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL, 0xF0F0F0F0A1LL, 0xF0F0F0F0A2LL }; // Radio pipe addresses for the 2 nodes to communicate.

bool onLine = true;
bool opdelijn;

int checkCounter = 0;
bool check1;
bool check2;
bool check3;
bool messageConfirmed = false;

float power = 0 ; // Process Variable value calculated to adjust speeds and keep on line
float kp = 80;  // This is the Proportional value. Tune this value to affect follow_line performance
float kd = 7;
float ki = 0.0040;

bool sensor0 = false;
bool sensor1 = false;
bool sensor2 = false;
bool sensor3 = false;
bool sensor4 = false;
bool sensor5 = false;

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
  Serial.println(F("Setup Done"));
  digitalWrite(dir_a, HIGH);
  digitalWrite(dir_b, LOW);
  check1 = false;
  check2 = false;
  check3 = false;
  MsTimer2::set(10, checkForNewTile);
  timerStarted = false;
} // end setup

int counter = 0;

void loop()
{
  sensor0 = readSensor(0);
  sensor1 = readSensor(1);
  sensor2 = readSensor(2);
  sensor3 = readSensor(3);
  sensor4 = readSensor(4);
  sensor5 = readSensor(5);

  Serial.print(sensor0);
  Serial.print(" ");
  
  Serial.print(sensor1);
  Serial.print(" ");
  
  Serial.print(sensor2);
  Serial.print(" ");
  
  Serial.print(sensor3);
  Serial.print(" ");
  
  Serial.print(sensor4);
  Serial.print(" ");
  
  Serial.print(sensor5);
  Serial.println(" ");
  
  checkToStartTimer();
  if (sensor0) {
    lastWideSensor = 0;
  }
  if (sensor5) {
    lastWideSensor = 5;
  }

  onLine = checkOnLine();

  if (onLine)
  {
    digitalWrite(dir_a, HIGH);
    digitalWrite(dir_b, LOW);
    followLine();
    counter = 0;
  }
  else {
    //checkDirections();
    turn();
  }
}  // end main loop

bool checkOnLine() {
  if (sensor1 || sensor2 || sensor3 || sensor4)
  {
    return true;
  }
  else {
    return false;
  }
}

void checkDirections() {
  if (sensor0)  {
    if (sensor5) {
      digitalWrite(dir_a, HIGH);
      digitalWrite(dir_b, LOW);
      analogWrite(pwm_a, maxMotorSpeed);
      analogWrite(pwm_b, maxMotorSpeed);
    }
  }
}

void turnLeft() {
  digitalWrite(dir_a, HIGH);
  digitalWrite(dir_b, HIGH);
  analogWrite(pwm_a, maxMotorSpeed);
  analogWrite(pwm_b, maxMotorSpeed);
}

void turnRight() {
  digitalWrite(dir_a, LOW);
  digitalWrite(dir_b, LOW);
  analogWrite(pwm_a, maxMotorSpeed);
  analogWrite(pwm_b, maxMotorSpeed);
}

void turn()
{
  if (lastWideSensor == 0)
  {
    if (counter > 100) {
      Serial.println(F("I can only go riiight"));
      sendLogMessage(6);
      if (direction == 3) {
        direction = 0;
      } else {
        direction++;
      }
      counter = 0;
    }

    turnRight();
  }

  if (lastWideSensor == 5) {
    if (counter > 100) {
      Serial.println(F("I can only go leeeeft"));
      sendLogMessage(5);
      if (direction == 0) {
        direction = 3;
      } else {
        direction--;
      }
      counter = 0;
    }

    turnLeft();
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

bool notCheckPIDSensors()
{
  if (!sensor1 && !sensor2 && !sensor3 && !sensor4) {
    return true;
  }
  else
  {
    return false;
  }
}

void checkToStartTimer() {
  if ((notCheckPIDSensors() || sensor0 || sensor5) && !timerStarted) {
    timerStarted = true;
    Serial.println(F("Started Timer..."));
    MsTimer2::start();
  }
}

void checkForNewTile() {
  checkCounter++;
  if (checkCounter < 50) {
    if (notCheckPIDSensors() && !check1)
    {
      check1 = true;
      //Serial.println("check1!");
    }

    if (sensor0 && !check2) {
      check2 = true;
      //Serial.println("check2!");
    }

    if (sensor5 && !check3) {
      check3 = true;
      //Serial.println("check3!");
    }

    if (check1 && check2 && check3) {
      check1 = false;
      check2 = false;
      check3 = false;
      Serial.println(F("Stopped Timer... Checks passed..."));
      timerStarted = false;
      sendLogMessage(direction);
      checkCounter = 0;
      MsTimer2::stop();
    }
  }
  else
  {
    check1 = false;
    check2 = false;
    check3 = false;
    timerStarted = false;
    checkCounter = 0;
    MsTimer2::stop();
    //Serial.println(F("Stopped Timer... Timed Out..."));
  }

}
void readLine() {
  for (int i = 0; i < 4; i++) {
    sensorReadings[i] = readSensor(sensor[i]);
    if (sensorReadings[i] == 1) {
      activeSensors += 1;
    }
    totalReading += sensorReadings[i] * (i + 1);
  }

  if (activeSensors == 0) {
    if (sensor0 || lastWideSensor == 0) {
      avgReading = -1;
      return;
    }
    if (sensor5 || lastWideSensor == 5) {
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

void calcPID() {
  readLine();

  previousError = error;
  error = avgReading - 2.5;
  totalError += error;

  power = (kp * error) + (kd * (error - previousError)) + (ki * totalError);
  //Serial.print(previousError);
  //Serial.print(" ");
  // Serial.print(error);
  // Serial.print(" ");
  // Serial.println(power);

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

unsigned long lastMessageId = -1;
unsigned long lastMessageTime = 0;

void sendLogMessage(unsigned long messageId) { 
    //Serial.println("Sending...");
    if(millis() - lastMessageTime > 250 || lastMessageId != messageId){
    myRadio.stopListening();
    unsigned long confirmId = 0;
    bool ok = myRadio.write(&messageId, sizeof(unsigned long));

    if (ok) {      
      //delay(2000);
      printf("Message Sent:  %d \n", messageId);      
      lastMessageId = messageId;
      myRadio.startListening();
      lastMessageTime = millis();

     /* if (myRadio.available()) {
        Serial.println("Listening for confirmation...");
        while (!messageConfirmed) {
          messageConfirmed = myRadio.read(&confirmId, sizeof(unsigned long));
          Serial.println(confirmId);
          Serial.println("Confirmed");
        }
      }*/
    }
  }
}



