/*  08/23/2015a

    Basic Line Following Robot using a Zagros Robotics Starter Kit and Pololu Line Sensor

    For wiring diagram see:
    http://www.zagrosrobotics.com/files/ZagrosLineMazeFollowWiring_08232015a.pdf

    For detailed instructions on installing the Arduino IDE, Redboard drivers,
    uploading sketches, and testing motors for proper wiring for this sketch see:
    http://www.zagrosrobotics.com/files/ZagrosArduinoInstallation_08232015a.pdf

    Parts list:

      1) Zagros Robot Starter Kit - Gobbit or Magician Version
         (http://www.zagrosrobotics.com/)
         Included components needed:
            Gobbit or Magician Robot Chassis with motors and battery holder
            Sparkfun Redboard
            Sparkfun Ardumoto Shield
            Jumper wires
            Breadboard optional

      2) Pololu QTR-8RC RC Reflectance Sensor Array
         (http://www.zagrosrobotics.com/shop/item.aspx?itemid=896)

      3) Black electrical tape Line course on white background

*/

//QTRSensors folder must be placed in your arduino libraries folder
#include <QTRSensors.h>  // Pololu QTR Library 
#include "printf.h"
#include "RF24.h"
#define rightMaxSpeed 150 // max speed of the robot
#define leftMaxSpeed 150 // max speed of the robot
#define rightBaseSpeed 100 // this is the speed at which the motors should spin when the robot is perfectly on the line
#define leftBaseSpeed 100 

//line sensor defines
#define NUM_SENSORS   4    // number of sensors used
#define NUM_SAMPLES_PER_SENSOR  4  // average 4 analog samples per sensor reading
#define TIMEOUT       2500  // waits for 2500 microseconds for sensor outputs to go low
#define EMITTER_PIN   QTR_NO_EMITTER_PIN  // emitter control pin not used.  If added, replace QTR_NO_EMITTER_PIN with pin#

// line sensor declarations
// sensors 0 through 7 are connected to digital pins 2 through 10, respectively (pin 3 is skipped and used by the Ardumoto controller)
// 0 is far Right sensor while 7 is far Left sensor
QTRSensorsAnalog qtrrc((unsigned char[]) {
  1, 2, 3, 4
}, NUM_SENSORS  );
unsigned int sensorValues[NUM_SENSORS]; // array with individual sensor reading values
unsigned int line_position = 0; // value from 0-7000 to indicate position of line between sensor 0 - 7

int lastError = 0;

int leftSensor = 0;
int rightSensor = 0;

int leftSensorWhiteValue;
int rightSensorWhiteValue;

int blackThreshold = 60;

RF24 myRadio(9, 10);

const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL }; // Radio pipe addresses for the 2 nodes to communicate.

boolean logging = true;

// ArduMoto motor driver vars
// pwm_a/b sets speed.  Value range is 0-255.  For example, if you set the speed at 100 then 100/255 = 39% duty cycle = slow
// dir_a/b sets direction.  LOW is Forward, HIGH is Reverse
int pwm_a = 5;  //PWM control for Ardumoto outputs A1 and A2 is on digital pin 10  (Left motor)
int pwm_b = 6;  //PWM control for Ardumoto outputs B3 and B4 is on digital pin 11  (Right motor)
int dir_a = 4;  //direction control for Ardumoto outputs A1 and A2 is on digital pin 12  (Left motor)
int dir_b = 7;  //direction control for Ardumoto outputs B3 and B4 is on digital pin 13  (Right motor)

// motor tuning vars
int calSpeed = 145;   // tune value motors will run while auto calibration sweeping turn over line (0-255)

// Proportional Control loop vars
float error = 0;
float previousError = 0;
float totalError = 0;
float PV = 0 ; // Process Variable value calculated to adjust speeds and keep on line
float kp = 0.1;  // This is the Proportional value. Tune this value to affect follow_line performance
float kd = 0.05;
float ki = 0.18;
int m1Speed = 0; // (Left motor)
int m2Speed = 0; // (Right motor)


void setup()
{

  Serial.begin(9600);
  Serial.println("Setup Begins");
  printf_begin();
  //Set control pins to be outputs
  pinMode(pwm_a, OUTPUT);
  pinMode(pwm_b, OUTPUT);
  pinMode(dir_a, OUTPUT);
  pinMode(dir_b, OUTPUT);

  //set both motors to stop
  analogWrite(pwm_a, 0);
  analogWrite(pwm_b, 0);

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
     if (i == 0 || i == 65) // slow sweeping turn right to pass sensors over line
      {
       digitalWrite(dir_a, LOW);
       analogWrite(pwm_a, calSpeed);
       digitalWrite(dir_b, LOW);
       analogWrite(pwm_b, calSpeed);
       printf("Right ");
      }

      else if (i == 25 || i == 85) // slow sweeping turn left to pass sensors over line
      {
       digitalWrite(dir_a, HIGH);
       analogWrite(pwm_a, calSpeed);
       digitalWrite(dir_b, HIGH);
       analogWrite(pwm_b, calSpeed);
       printf("Left ");
      }

    qtrrc.calibrate(); // reads all sensors with the define set 2500 microseconds (25 milliseconds) for sensor outputs to go low.
    //printf("Left sensor value: %d Mid sensor value: %d Right sensor value %d \n", sensorValues[0], sensorValues[1], sensorValues[2]);
  }  // end calibration cycle
  // printf("Left sensor value: %d Mid sensor value: %d Right sensor value %d \n" , sensorValues[0], sensorValues[1], sensorValues[2]);

  // read calibrated sensor values and obtain a measure of the line position from 0 to 7000
  line_position = qtrrc.readLine(sensorValues);
  Serial.println(line_position);
  // read the value of only a single sensor to see the line.
  // when the value is greater than 200 the sensor sees the line.
  while (sensorValues[3] < 200)  // wait for line position to near center
  {
    line_position = qtrrc.readLine(sensorValues);
    Serial.println(line_position);
  }

  // find near center
  while (line_position > 1650) // continue loop until line position is near center
  {
    line_position = qtrrc.readLine(sensorValues);
    Serial.println(line_position);
    Serial.println("Line is at center");
  }

  // stop both motors
  analogWrite(pwm_b, 0); // stop right motor first which kinda helps avoid over run
  analogWrite(pwm_a, 0);

  // delay as indicator setup and calibration is complete
  delay(1000);
  Serial.println(line_position);
  for (int i = 0; i < NUM_SENSORS; i++)
  {
    Serial.print(qtrrc.calibratedMinimumOn[i]);
    Serial.print(' ');
  }
  Serial.println();

  // print the calibration maximum values measured when emitters were on
  for (int i = 0; i < NUM_SENSORS; i++)
  {
    Serial.print(qtrrc.calibratedMaximumOn[i]);
    Serial.print(' ');
  }


  Serial.println("Setup Done");
} // end setup



void loop() // main loop
{


  // read calibrated sensor values and obtain a measure of the line position from 0 to 7000
  unsigned int line_position = qtrrc.readLine(sensorValues);
  // printf("Left sensor value: %d Mid sensor value: %d Right sensor value %d \n" , sensorValues[0], sensorValues[1], sensorValues[2]);
  // begin line
  //readWideSensors();
  follow_line(line_position);



  //
}  // end main loop



// line following subroutine
//  Proportional Control Only
//  For a lesson in more advanced PID control, see the Zagros Tutorial on the Proportional-Derivative Control
//  http://www.instructables.com/id/Basic-PD-Proportional-Derivative-Line-Follower/

void follow_line(int line_position) //follow the line
{  
  int error = line_position - 1500;

  int motorSpeed = kp * error + kd * (error - lastError) + ki * totalError;
  lastError = error;

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
      
  
      Serial.print("Process Variable: ");
      Serial.print(PV);
      Serial.print(" Line position: ");
      Serial.print(line_position);
      Serial.print(" Motor 1: ");
      Serial.print(rightMotorSpeed);
      Serial.print(" Motor 2: ");
      Serial.println(leftMotorSpeed);

} // end follow_line

void readWideSensors()
{
  leftSensor = analogRead(A4);
  rightSensor = analogRead(A0);

  leftSensor = ((double)leftSensor / 1024) * 100.0;
  rightSensor = ((double)rightSensor / 1024) * 100.0;
  detectTile(leftSensor, rightSensor);
}

void detectTile(int left, int right)
{

  if (left > blackThreshold)
  {
    if (right > blackThreshold)
    { //bb
      readTile(3, 5);
      // printf("Black Black \n");

    }
    else
    { //bw
      readTile(1, 4);
      line_position = 0;
      //  printf("Black White \n");
    }
  }
  else

  { //wb
    if (right > blackThreshold)
    {
      // line_position = 2000;
      readTile(0, 0);
      //     printf("White Black \n");
    } else
    {
      readTile(0, 0);
      //   printf("White White \n");
    }
  }
  //    detectEnd();

}



void readTile(int type, int caseId)
{
  int arrivalDirection = -1;

  switch (caseId)
  {
    case 0: // Arrived from right side of the corner.
      if (logging)
      {
        sendLogMessage(0);
      }
      //  arrivalDirection = orientation + 1;
      //addTile(1, orientation + 1, xPosition, yPosition, orientation);
      // moveWideLeft();
      break;
    case 1: // Arrived from left side of the corner.
      if (logging)
      {
        sendLogMessage(1);
      }
      // arrivalDirection = orientation;
      // moveWideRight();
      //addTile(1, orientation, xPosition, yPosition, orientation);
      break;

    case 2: // Arrived from bottom side of a T tile.
      if (logging)
      {
        sendLogMessage(2);
      }
      //  moveWideLeft();
      //addTile(2, orientation + 1, xPosition, yPosition, orientation);
      // arrivalDirection = orientation + 1;
      break;

    case 3: // Arrived from left side of the T.
      if (logging)
      {
        sendLogMessage(3);
      }
      //addTile(2, orientation, xPosition, yPosition, orientation);
      //  arrivalDirection = orientation;
      //  moveWideRight();
      break;

    case 4: // Arrived from right side of the T.
      if (logging)
      {
        sendLogMessage(4);
      }
      //addTile(2, orientation + 2, xPosition, yPosition, orientation);
      //  arrivalDirection = orientation + 2;
      //  moveWideLeft();
      break;

    case 5: // Arrived at an intersection.
      if (logging)
      {
        sendLogMessage(5);
      }
      //addTile(3, 0, xPosition, yPosition, orientation);
      // arrivalDirection = 0;
      //  moveWideLeft();
      break;

    case 6:
      if (logging)
      {
        sendLogMessage(8);
      }

      //  moveStraight();
      break;
  }



  //makeMove();
}

int lastMessageId = -1;

void sendLogMessage(int messageId) {

  if (messageId != lastMessageId) {
    myRadio.stopListening();
    unsigned long id = messageId;
    bool ok = myRadio.write(&id, sizeof(unsigned long));
    if (ok) {
      printf("Message Send:  %d \n", messageId);
    } else {
      printf("Failed to send... \n");
    }
    lastMessageId = messageId;
  }
  myRadio.startListening();
}




