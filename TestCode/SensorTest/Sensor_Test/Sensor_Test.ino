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
  double detectedValue = (double) sensor0 / (double) range;  
  bool value = false;
  if(detectedValue > 0.8){
    value = true;
  }
  Serial.print(detectedValue);
  Serial.print(" ");
  
  int sensor1 = analogRead(A1);
  detectedValue = (double) sensor1 / (double) range;  
  value = false;
  if(detectedValue > 0.8){
    value = true;
  }
  Serial.print(detectedValue);
  Serial.print(" ");

  int sensor2 = analogRead(A2);
  detectedValue = (double) sensor2 / (double) range;  
  value = false;
  if(detectedValue > 0.8){
    value = true;
  }
  Serial.print(detectedValue);
  Serial.print(" ");
  
  int sensor3 = analogRead(A3);
  detectedValue = (double) sensor3 / (double) range;  
  value = false;
  if(detectedValue > 0.8){
    value = true;
  }
  Serial.print(detectedValue);
  Serial.print(" ");
  
  int sensor4 = analogRead(A4);
  detectedValue = (double) sensor4 / (double) range;  
  value = false;
  if(detectedValue > 0.8){
    value = true;
  }
  Serial.print(detectedValue);
  Serial.println(" ");  

  int sensor5 = analogRead(A5);
  detectedValue = (double) sensor5 / (double) range;  
  value = false;
  if(detectedValue > 0.8){
    value = true;
  }
  Serial.print(detectedValue);
  Serial.print(" ");
}
