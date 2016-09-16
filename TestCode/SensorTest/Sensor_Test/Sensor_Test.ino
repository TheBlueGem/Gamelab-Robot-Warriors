#include <SPI.h>
#include "RF24.h"
#include "printf.h"

int sensor1WhiteValue;
int sensor2WhiteValue;
int sensor3WhiteValue;

void setup() {
  Serial.begin(9600);
  printf_begin();
  // put your setup code here, to run once:
  
  for(int i = 0; i < 10; i++){
  sensor1WhiteValue += analogRead(A0);
  sensor2WhiteValue += analogRead(A1);
  sensor3WhiteValue += analogRead(A2);
  printf("value : %d\n", sensor3WhiteValue);
  }
  
  sensor1WhiteValue = sensor1WhiteValue / 10;
  sensor2WhiteValue = sensor2WhiteValue / 10;
  sensor3WhiteValue = sensor3WhiteValue / 10;
  
  printf("White value sensor 1: %d\n", sensor1WhiteValue);
  printf("White value sensor 2: %d\n", sensor2WhiteValue);
  printf("White value sensor 3: %d\n", sensor3WhiteValue);
}

void loop() {
  delay(1000);
  int sensor1 = analogRead(A0);
  int detectedValue = sensor1 - sensor1WhiteValue;
  int sensorRange = 1024 - sensor1WhiteValue;
   printf("Sensor 1.....Detected value: %d ", detectedValue);
  int percentileValue = ((double) detectedValue / (double) sensorRange) * 100.0;
  printf("Percentage: %d\n", percentileValue);

  int sensor2 = analogRead(A1);
  detectedValue = sensor2 - sensor2WhiteValue;
  sensorRange = 1024 - sensor2WhiteValue;
   printf("Sensor 2.....Detected value: %d ", detectedValue);
  percentileValue = ((double) detectedValue / (double) sensorRange) * 100.0;
  printf("Percentage: %d\n", percentileValue);

  int sensor3 = analogRead(A2);
  detectedValue = sensor3 - sensor3WhiteValue;
  sensorRange = 1024 - sensor3WhiteValue;
  printf("Sensor 3.....Detected value: %d ", detectedValue);
  percentileValue = ((double) detectedValue / (double) sensorRange) * 100.0;
  printf("Percentage: %d\n\n", percentileValue);

  
}
