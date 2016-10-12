#include <SPI.h>
#include "RF24.h"
#include "printf.h"

void setup() {
  Serial.begin(9600);
  printf_begin();
  // put your setup code here, to run once: 
 
}

void loop() {
  double range = 1024;

  int sensor0 = analogRead(A0);
  double detectedValue0 = (double) sensor0 / (double) range;   
  
  int sensor1 = analogRead(A1);
  double detectedValue1 = (double) sensor1 / (double) range;  
  //Serial.print(detectedValue1);
  //Serial.print(" ");

  int sensor2 = analogRead(A2);
  double detectedValue2 = (double) sensor2 / (double) range;  
  
  
  int sensor3 = analogRead(A3);
  double detectedValue3 = (double) sensor3 / (double) range;  
  
  
  int sensor4 = analogRead(A4);
  double detectedValue4 = (double) sensor4 / (double) range;  
  //Serial.print(detectedValue4);
  //Serial.print(" ");  

  int sensor5 = analogRead(A5);
  double detectedValue5 = (double) sensor5 / (double) range;
  
  Serial.print(detectedValue0);
  Serial.print(" ");  
  Serial.print(detectedValue2);
  Serial.print(" ");
  Serial.print(detectedValue3);
  Serial.print(" ");
  Serial.print(detectedValue5);
  Serial.println(" ");
}
