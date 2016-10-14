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

int direction = 10; // 0 = Robot North, 1 = Robot East, 2 = Robot South, 3 = Robot West.

bool c = false;
int lastWideSensor = 9;

float error = 0;
float previousError = 0;
float totalError = 0;

RF24 myRadio(9, 10);

// Topology
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL }; // Radio pipe addresses for the 2 nodes to communicate.

bool onLine = true;
bool opdelijn;

float power = 0 ; // Process Variable value calculated to adjust speeds and keep on line
float kp = 80;  // This is the Proportional value. Tune this value to affect follow_line performance
float kd = 7;
float ki = 0.0040;

bool timer;
int timerCount;
bool sensor0 = false;
bool sensor1 = false;
bool sensor2 = false;
bool sensor3 = false;
bool sensor4 = false;
bool sensor5 = false;

bool turningRight = false;
bool turningLeft = false;
bool checkingForDeadEnd = false;
int turnCounter = 0;
bool deadEndPossible = false;
int noDeadEndCounter = 0;
int deadEndCounter = 0;
bool deadEndTurn = false;


unsigned long lastDirectionChange = 0;

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
  sensor0 = readSensor(0);
  sensor1 = readSensor(1);
  sensor2 = readSensor(2);
  sensor3 = readSensor(3);
  sensor4 = readSensor(4);
  sensor5 = readSensor(5);
  if (deadEndTurn && turnCounter < 200) {
    if (turnCounter < 65) {
      straight();
    }
    else
    {
      turnRight();
    }
    turnCounter++;
  }
  else if ((turningRight || turningLeft) && turnCounter < 165) {
    if (turnCounter < 65) {
      straight();
    }
    else
    {
      if (turningRight) {
        turnRight();
      }
      else
      {
        turnLeft();
      }
    }

    turnCounter++;
    /*if(sensor2 || sensor3){
      turningRight = false;
      turningLeft = false;
      turnCounter = 0;
      }*/
  }
  else
  {
    deadEndTurn = false;
    turningRight = false;
    turningLeft = false;
    checkingForDeadEnd = false;
    turnCounter = 0;

    checkForNewTile();

    onLine = checkPIDSensors();

    if (deadEndPossible && (sensor0 || sensor5)) {
      //sendLogMessage(1);
      deadEndPossible = false;
      noDeadEndCounter = 0;
      deadEndCounter = 0;
    }

    if (sensor0) {
      turningRight = true;

      if ((millis() - lastDirectionChange) > 1000) {
        lastDirectionChange = millis();

        if (direction == 13) {
          direction = 10;
        } else {
          direction++;
        }
        sendLogMessage(direction);
      }
    }
    else if (checkPIDSensors())
    {
      digitalWrite(dir_a, HIGH);
      digitalWrite(dir_b, LOW);

      deadEndCounter = 0;
      followLine();
    }
    else if (sensor5) {
      turningLeft = true;
      if ((millis() - lastDirectionChange) > 1000) {
        lastDirectionChange = millis();

        if (direction == 10) {
          direction = 13;
        } else {
          direction--;
        }
        sendLogMessage(direction);
      }
    }
    else if (deadEndPossible)
    {
      checkForDeadEnd();
    }
  }

  if (!deadEndPossible && noDeadEndCounter < 500) {
    noDeadEndCounter++;
  }
  else
  {
    if (!deadEndPossible ) {
      deadEndPossible = true;
      //sendLogMessage(2);
    }
  }

} // end main loop

void checkForDeadEnd() {
  if (!deadEndTurn) {
    if (!sensor0 && !sensor1 && !sensor2 && !sensor3 && !sensor4 && !sensor5)
    {
      deadEndCounter++;
    }
    else
    {
      deadEndCounter = 0;
    }

    if (deadEndCounter >= 100) {
      deadEndPossible = false;
      deadEndCounter = 0;
      sendLogMessage(7);
      deadEndTurn = true;
    }

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

void straight() {
  digitalWrite(dir_a, HIGH);
  digitalWrite(dir_b, LOW);
  analogWrite(pwm_a, maxMotorSpeed);
  analogWrite(pwm_b, maxMotorSpeed);

}

void turnLeft() {
  digitalWrite(dir_a, HIGH);
  digitalWrite(dir_b, HIGH);
  analogWrite(pwm_a, maxMotorSpeed - 20);
  analogWrite(pwm_b, maxMotorSpeed + 20);
}

void turnRight() {
  digitalWrite(dir_a, LOW);
  digitalWrite(dir_b, LOW);
  analogWrite(pwm_a, maxMotorSpeed + 20);
  analogWrite(pwm_b, maxMotorSpeed - 20);
}

void turn()
{
  if (lastWideSensor == 0)
  {
    if (!timer) {
      if ((millis() - lastDirectionChange) > 1000) {
        lastDirectionChange = millis();

        if (direction == 13) {
          direction = 10;
        } else {
          direction++;
        }
        sendLogMessage(direction);
      }
    }
    turnRight();
  }

  if (lastWideSensor == 5)
  {
    if (!timer) {
      if ((millis() - lastDirectionChange) > 1000) {
        lastDirectionChange = millis();

        if (direction == 10) {
          direction = 13;
        } else {
          direction--;
        }
        sendLogMessage(direction);
      }
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

int whiteCount = 0;
bool whiteAvailable = true;

bool checkForNewTile() {
  if (!timer && !sensor1 && !sensor2 && !sensor3 && !sensor4)
  {
    timer = true;
    Serial.println("Timer Started...");
  }

  if (timer) {
    if (!sensor1 && !sensor2 && !sensor3 && !sensor4 && whiteAvailable) {
      whiteCount++;
      whiteAvailable = false;
      Serial.print("White Count: ");
      Serial.println(whiteCount);
    }
    else if (checkPIDSensors()) {
      whiteAvailable = true;
    }
  }

  if (timer) {
    timerCount++;
    if (!sensor0 && !sensor5 && checkPIDSensors() && timerCount <= 500) {
      if (whiteCount == 3) {
        whiteCount = 0;
        sendLogMessage(6);
        timerCount = 0;
        timer = false;
        whiteAvailable = true;
        Serial.println("Timer Stopped Straight Tile Found...");
        return true;
      }
    }
    if (timerCount > 100 && whiteCount == 1 && !sensor1 && !sensor2 && !sensor3 && !sensor4) {
      //Serial.println("Dead End...");
    }
    if (timerCount > 500) {
      timer = false;
      whiteCount = 0;
      timerCount = 0;
      Serial.println("Timer Stopped Timeout...");
      return false;
    }
  }
}

bool checkPIDSensors() {
  if (sensor1 || sensor2 || sensor3 || sensor4) {
    return true;
  }
  else
  {
    return false;
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

float readSensorValue(int sensor)
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
  return detectedValue;
}
