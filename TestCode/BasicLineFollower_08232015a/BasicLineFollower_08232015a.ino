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

//line sensor defines
#define NUM_SENSORS   6     // number of sensors used
#define NUM_SAMPLES_PER_SENSOR  4  // average 4 analog samples per sensor reading
#define TIMEOUT       2500  // waits for 2500 microseconds for sensor outputs to go low
#define EMITTER_PIN   QTR_NO_EMITTER_PIN  // emitter control pin not used.  If added, replace QTR_NO_EMITTER_PIN with pin#

// line sensor declarations
// sensors 0 through 7 are connected to digital pins 2 through 10, respectively (pin 3 is skipped and used by the Ardumoto controller)
// 0 is far Right sensor while 7 is far Left sensor
QTRSensorsAnalog qtrrc((unsigned char[]) {5, 4, 3, 2, 1, 0}, NUM_SENSORS  );
unsigned int sensorValues[NUM_SENSORS]; // array with individual sensor reading values
unsigned int line_position = 0; // value from 0-7000 to indicate position of line between sensor 0 - 7

// ArduMoto motor driver vars
// pwm_a/b sets speed.  Value range is 0-255.  For example, if you set the speed at 100 then 100/255 = 39% duty cycle = slow
// dir_a/b sets direction.  LOW is Forward, HIGH is Reverse
int pwm_a = 5;  //PWM control for Ardumoto outputs A1 and A2 is on digital pin 10  (Left motor)
int pwm_b = 6;  //PWM control for Ardumoto outputs B3 and B4 is on digital pin 11  (Right motor)
int dir_a = 4;  //direction control for Ardumoto outputs A1 and A2 is on digital pin 12  (Left motor)
int dir_b = 7;  //direction control for Ardumoto outputs B3 and B4 is on digital pin 13  (Right motor)

// motor tuning vars
int calSpeed = 255;   // tune value motors will run while auto calibration sweeping turn over line (0-255)

// Proportional Control loop vars
float error = 0;
float PV = 0 ; // Process Variable value calculated to adjust speeds and keep on line
float kp = 1;  // This is the Proportional value. Tune this value to affect follow_line performance
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

 

  // delay to allow you to set the robot on the line, turn on the power,
  // then move your hand away before it begins moving.
  delay(2000);

  // calibrate line sensor; determines min/max range of sensed values for the current course
  for (int i = 0; i <= 100; i++)  // begin calibration cycle to last about 2.5 seconds (100*25ms/call)
  {
   
         
    // auto calibration sweeping left/right, tune 'calSpeed' motor speed at declaration
    // just high enough all sensors are passed over the line. Not too fast.
  /*  if (i == 0 || i == 60) // slow sweeping turn right to pass sensors over line
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
    }*/

    qtrrc.calibrate(); // reads all sensors with the define set 2500 microseconds (25 milliseconds) for sensor outputs to go low.
    //printf("Left sensor value: %d Mid sensor value: %d Right sensor value %d \n", sensorValues[0], sensorValues[1], sensorValues[2]);
  }  // end calibration cycle
    // printf("Left sensor value: %d Mid sensor value: %d Right sensor value %d \n" , sensorValues[0], sensorValues[1], sensorValues[2]);

  // read calibrated sensor values and obtain a measure of the line position from 0 to 7000
  line_position = qtrrc.readLine(sensorValues);
  Serial.println(line_position);
  // read the value of only a single sensor to see the line.
  // when the value is greater than 200 the sensor sees the line.
  while (sensorValues[5] < 200)  // wait for line position to near center
  {
    line_position = qtrrc.readLine(sensorValues);
      Serial.println(line_position);
  }

  // find near center
  while (line_position > 3500) // continue loop until line position is near center
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
  follow_line(line_position);
 //
 //delay(1000);
}  // end main loop



// line following subroutine
//  Proportional Control Only
//  For a lesson in more advanced PID control, see the Zagros Tutorial on the Proportional-Derivative Control
//  http://www.instructables.com/id/Basic-PD-Proportional-Derivative-Line-Follower/

void follow_line(int line_position) //follow the line
{

  // 0 is far Right sensor while 7 (7000 return) is far Left sensor

  switch (line_position)
  {


    // Line has moved off the left edge of sensor
    // This will make it turn fast to the left
    case 5000:
           digitalWrite(dir_a, HIGH); 
           analogWrite(pwm_a, 200);
           digitalWrite(dir_b, HIGH);  
           analogWrite(pwm_b, 200);
        Serial.println(line_position);
        Serial.println("Turning Left");
      break;

    // Line had moved off the left edge of sensor

    // Line had moved off the right edge of sensor
    // This will make it turn fast to the right

    case 0:     
           digitalWrite(dir_a, LOW); 
           analogWrite(pwm_a, 200);
           digitalWrite(dir_b, LOW);  
           analogWrite(pwm_b, 200);

        Serial.println(line_position);
        Serial.println("Turning Right");
      break;

    // The line is still within the sensors.
    // This will calculate adjusting speed to keep the line in center.
    default:
      error = (float)line_position - 2500; // 3500 is center measure of 7000 far left and 0 on far right

      // This sets the motor speed based on a proportional only formula.
      // kp is the floating-point proportional constant you need to tune.
      // Maybe start with a kp value around 1.0, tuned in declared Proportional Control loop vars at the top of this code.
      // Note that it's very important you get your signs right, or else the
      // control loop will be unstable.

      // calculate the new Process Variable
      // this is the value that will be used to alter the speeds
      PV = kp * error;

      // this section limits the PV (motor speed pwm value)
      // limit PV to 55
      if (PV > 55)
      {
        PV = 55;
      }

      if (PV < -55)
      {
        PV = -55;
      }

      // adjust motor speeds to correct the path
      // Note that if PV > 0 the robot needs to turn left
      m1Speed = 200 - PV;
      m2Speed = 200 + PV;
     
      //set motor speeds
      digitalWrite(dir_a, HIGH);  
      analogWrite(pwm_a, m1Speed);
      digitalWrite(dir_b, LOW);  
      analogWrite(pwm_b, m2Speed);

      Serial.println("Going straight");
      Serial.println(line_position);
      break;
  }

} // end follow_line



