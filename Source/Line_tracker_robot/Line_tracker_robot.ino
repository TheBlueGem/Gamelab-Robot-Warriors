#include <SPI.h>
#include "RF24.h"
#include "Tile.h"
#include "printf.h"

// digital pins used: 2 = rf; 4,5,6,7 = motorshield; 3,11,12,13 = radio
// analog pins used: 0 = sensor 1; 1 = sensor 2; 2 = sensor 3; 3 = sensor 4; 4 = sensor 5;

Tile* tiles[50];
int robotOrientation[50];

int amountOfTiles = 0;
int amountOfPackages = 0;

RF24 myRadio (2, 3);
byte addresses[][6] = {"0"};

// Topology
const uint64_t pipes[2] = { 0xABCDABCD71LL, 0x544d52687CLL };              // Radio pipe addresses for the 2 nodes to communicate.

struct package {
  int id;
  int tile;
  int tileOrientation;
  int xCoordinate;
  int yCoordinate;
  int robotOrientation;
};

typedef struct package Package;

int packageId = 0;
int loopsWithoutListening = 0;

int orientation = 0; // 0 = north, 1 = east, 2 = south, 3 = west
int xAxis = 0;
int yAxis = 0;

int followLineDifference = 3;
int blackThreshold = 50;

int sensor1WhiteValue;
int sensor2WhiteValue;
int sensor3WhiteValue;
boolean logging = true;
int engineStatus; // 0 = off, 1 = straight, 2 = reverse, 3 = left, 4 = wideLeft, 5 = right, 6 = wideRight
int junction = 0; //followLine = 0, left = 1, straight = 2, right = 3, reverse = 4

void Motor1(int pwm, boolean reverse) {
  analogWrite(6, pwm); //set pwm control, 0 for stop, and 255 for maximum speed
  if (reverse) {
    digitalWrite(7, HIGH);
  }
  else {
    digitalWrite(7, LOW);
  }  
}

void Motor2(int pwm, boolean reverse) {
  analogWrite(5, pwm);
  if (reverse) {
    digitalWrite(4, HIGH);
  }
  else {
    digitalWrite(4, LOW);
  }
}

void setup() {
  int i;
  for (i = 4; i <= 7; i++) //For Arduino Motor Shield
    pinMode(i, OUTPUT);  //set pin 4,5,6,7 to output mode


  Serial.begin(57600);

  myRadio.begin();
  myRadio.setAutoAck(1);
  myRadio.enableAckPayload();
  myRadio.setRetries(0, 15);
  //myRadio.setDataRate( RF24_250KBPS ) ;
  myRadio.openWritingPipe(pipes[1]);        // Both radios listen on the same pipes by default, and switch when writing
  myRadio.openReadingPipe(1, pipes[0]);
  myRadio.startListening();
  Serial.println("Setup");

  //sensor1White = analogRead(A0);
  //sensor2White = analogRead(A1);
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

void findLine() {
  int lineFound = 0;
  while (lineFound < 1000) {
    int sensor1 = readSensor(1);
    int sensor2 = readSensor(2);
    int sensor3 = readSensor(3);
    if (sensor2 > 70) {
      lineFound++;
    }
    if (sensor1 - followLineDifference > sensor3) {
      moveWideLeft();
      //Serial.println("sensor1 valued ");
    } else if (sensor3 - followLineDifference > sensor1) {
      moveWideRight();
      //Serial.println("sensor3 valued");
    } else {
      moveStraight();
      //Serial.println("sensor2 valued");
    }
  }
  if (logging) {
    Serial.println("Line Found");
  }
  junction = 0;
  moveStraight();
}

int readSensor(int sensor) { //returns a percentage of how black the sensor is. 0 = white; 100 = black.
  double percentage;
  int detectedValue;
  int range;
  int sensorValue;
  switch (sensor) {
    case 1: // read sensor 1.
      sensorValue = analogRead(A0);
      if (sensorValue < sensor1WhiteValue) {
        sensor1WhiteValue = sensorValue;
      }
      detectedValue = sensorValue - sensor1WhiteValue;
      range = 1024 - sensor1WhiteValue;
      break;
    case 2: // read sensor 2.
      sensorValue = analogRead(A1);
      if (sensorValue < sensor2WhiteValue) {
        sensor2WhiteValue = sensorValue;
      }
      detectedValue = sensorValue - sensor2WhiteValue;
      range = 1024 - sensor2WhiteValue;
      break;
    case 3: // read sensor 3.
      sensorValue = analogRead(A2);
      if (sensorValue < sensor3WhiteValue) {
        sensor3WhiteValue = sensorValue;
      }
      detectedValue = sensorValue - sensor3WhiteValue;
      range = 1024 - sensor3WhiteValue;
      break;
  }
    
  if (detectedValue < range) {
    percentage = ((double) detectedValue / (double) range) * 100.0;
  }
  return (int) percentage;
}

void setNewLocation() {
  switch (orientation) {
    case 0:
      yAxis++;
      break;
    case 1:
      xAxis++;
      break;
    case 2:
      yAxis--;
      break;
    case 3:
      xAxis--;
      break;
  }
}

void moveStraight() {
  if (engineStatus != 1) {
    Motor1(255, false);
    Motor2(255, true);
    engineStatus = 1;
  }
}

void moveLeft() {
  if (engineStatus != 3) {
    Motor1(255, true);
    Motor2(255, true);
    engineStatus = 3;
  }
}

void moveWideLeft() {
  if (engineStatus != 4) {
    Motor1(0, true);
    Motor2(255, true);
    engineStatus = 3;
  }
}

void moveRight() {
  if (engineStatus != 5) {
    Motor1(255, false);
    Motor2(255, false);
    engineStatus = 4;
  }
}

void moveWideRight() {
  if (engineStatus != 6) {
    Motor1(255, false);
    Motor2(0, false);
    engineStatus = 4;
  }
}

void turnOff() {
  Motor1(0, false);
  Motor2(0, true);
  engineStatus = 0;
  if (logging) {
    Serial.println("Turned Off");
  }
}

void addTile(int tileType, int tileOrientation, int x, int y, int rOrientation) {
  Tile* newTile = new Tile(tileType, tileOrientation, x, y);
  //sendMessage(newTile->getType(), newTile->getOrientation(), x, y, orientation);
  boolean tileKnown = false;
  for (int i = 0; i < amountOfTiles; i++) {
    if (x == tiles[i]->getXCoordinate() && y == tiles[i]->getYCoordinate()) { //check if there is a tile on x and y coordinates.
      newTile = tiles[i];
      tileKnown = true;
    }
  }
  if (!tileKnown) {
    for (int i = 0; i < amountOfTiles; i++) {
      Tile* checkingTile = tiles[i];
      int x2 = checkingTile->getXCoordinate();
      int y2 = checkingTile->getYCoordinate();
      if (x == x2 && y + 1 == y2) { //CheckingTile is north of newTile
        newTile->setNorth(checkingTile);
        checkingTile->setSouth(newTile);
      }
      if (x + 1 == x2 && y == y2) { //CheckingTile is east of newTile
        newTile->setEast(checkingTile);
        checkingTile->setWest(newTile);
      }
      if (x == x2 && y - 1 == y2) { //CheckingTile is south of newTile
        newTile->setSouth(checkingTile);
        checkingTile->setNorth(newTile);
      }
      if (x - 1 == x2 && y == y2) { //CheckingTile is west of newTile
        newTile->setWest(checkingTile);
        checkingTile->setEast(newTile);
      }
    }

    bool north = checkDirectionPossibility(tileType, tileOrientation, 0);
    bool east = checkDirectionPossibility(tileType, tileOrientation, 1);
    bool south = checkDirectionPossibility(tileType, tileOrientation, 2);
    bool west = checkDirectionPossibility(tileType, tileOrientation, 3);

    newTile->setOptionNorth(north);
    newTile->setOptionSouth(south);
    newTile->setOptionEast(east);
    newTile->setOptionWest(west);

    //sendMessage(packageId, x, y, (String) ammountOfTiles);
  }
  tiles[amountOfTiles] = newTile;
  robotOrientation[amountOfTiles] = rOrientation;
  amountOfTiles++;
}

void detectIntersection() {
  Motor1(0, false);
  Motor2(0, true);
  delay(50);
  int s1 = readSensor(1);
  int s2 = readSensor(2);
  int s3 = readSensor(3);
  if (s1 > blackThreshold) {
    if (s2 > blackThreshold) {
      if (s3 > blackThreshold) { //bbb
        intersection();
      } else { //bbw
        tJunction(2);
      }
    } else {
      if (s3 > blackThreshold) { //bwb
        tJunction(0);
      } else { //bww
        corner(0);
      }
    }
  } else {
    if (s2 > blackThreshold) {
      if (s3 > blackThreshold) { //wbb
        tJunction(1);
      } else { //wbw
        if (logging) {
          Serial.println("follow");
        }
      }
    } else {
      if (s3 > blackThreshold) { //wwb
        corner(1);
      } else { //www
        detectEnd();
      }
    }
  }
}

void detectEnd() {
  turnOff();
  delay(500);
  moveStraight();
  delay(150);
  turnOff();
  delay(100);
  int s1 = readSensor(1);
  int s2 = readSensor(2);
  int s3 = readSensor(3);
  if (s1 > blackThreshold && s2 > blackThreshold && s3 > blackThreshold) {
    if (logging) {
      Serial.println("Finished");
    }
    addTile(4, orientation, xAxis, yAxis, orientation); //einde dus alle richtingen false zetten.
    sendMessage(4, orientation, xAxis, yAxis, orientation);
  } else {
    if (logging) {
      Serial.println("deadEnd");
    }
    addTile(0, orientation, xAxis, yAxis, orientation);
    sendMessage(0, orientation, xAxis, yAxis, orientation);
    makeMove();
  }
}

void corner(int side) {
  switch (side) {
    case 0: // arrive from right side of the ^.
      if (logging) {
        Serial.println("corner0");
      }
      addTile(1, orientation + 1, xAxis, yAxis, orientation);
      sendMessage(1, orientation + 1, xAxis, yAxis, orientation);
      turnOff();
      break;
    case 1: // arrive from left side of the ^.
      if (logging) {
        Serial.println("corner1");
      }
      addTile(1, orientation, xAxis, yAxis, orientation);
      sendMessage(1, orientation, xAxis, yAxis, orientation);
      turnOff();
      break;
  }
  makeMove();
}

void tJunction(int side) {
  switch (side) {
    case 0: // arrive from bottom side of the T.
      if (logging) {
        Serial.println("tJunction0");
      }

      addTile(2, orientation + 1, xAxis, yAxis, orientation);
      sendMessage(2, orientation + 1, xAxis, yAxis, orientation);
      turnOff();
      break;
    case 1: // arrive from left side of the T.
      if (logging) {
        Serial.println("tJunction1");
      }

      addTile(2, orientation, xAxis, yAxis, orientation);
      sendMessage(2, orientation, xAxis, yAxis, orientation);
      turnOff();
      break;
    case 2: // arrive from right side of the T.
      if (logging) {
        Serial.println("tJunction2");
      }
      addTile(2, orientation + 2, xAxis, yAxis, orientation);
      sendMessage(2, orientation + 2, xAxis, yAxis, orientation);
      //sendMessageWithBool(packageId, 2, orientation + 2, "tJunction2+bool", north, east, south, west);
      turnOff();
      break;
  }
  makeMove();
}

void intersection() {
  if (logging) {
    Serial.println("intersection");
  }
  addTile(3, 0, xAxis, yAxis, orientation);
  sendMessage(3, 0, xAxis, yAxis, orientation);
  turnOff();
  makeMove();
}

void sendMessage(int tile, int tileOrientation, int x, int y, int robotOrientation) {
  bool received = false;
  // First, stop listening so we can talk.
  Package data = {packageId, tile, tileOrientation, x, y, robotOrientation};
  //receiveMessage();
  myRadio.openWritingPipe(pipes[0]);
  myRadio.openReadingPipe(1, pipes[1]);
  myRadio.stopListening();
  /*while (!received) {
    if (!myRadio.write(&data, sizeof(data))) {
      Serial.println("failed.");
    } else {
      received = true;
    }
  }*/
  myRadio.openWritingPipe(pipes[1]);
  myRadio.openReadingPipe(1, pipes[0]);
  myRadio.startListening();
  //receiveMessage();
}

void receiveMessage() {
  Serial.println("startListening");
  Package data;
  /*while (myRadio.available()) {
    Serial.println("readPackage");
    myRadio.read(&data, sizeof(data) );
    if (data.tile >= 0) {
      Serial.println("addTile");
      data.id = -1;
      //myRadio.writeAckPayload(pipes[1],&data, sizeof(data) );
      Serial.print("R: ");
      Serial.print("id: ");
      Serial.print(data.id);
      Serial.print("Tile: ");
      Serial.print(data.tile);
      Serial.print(", tOrientation: ");
      Serial.print(data.tileOrientation);
      Serial.print(", x: ");
      Serial.print(data.xCoordinate);
      Serial.print(", y: ");
      Serial.print(data.yCoordinate);
      Serial.print(", rOrientation: ");
      Serial.println(data.robotOrientation);
      addTile(data.tile, data.tileOrientation, data.xCoordinate, data.yCoordinate, data.robotOrientation);
    }
  }*/
}

//make a choice based on the orientation of the robot, tile type and tile orientation. Always try to go right, if not go straight, if not go left or turn around.
void makeMove() {
  //get current tile
  Tile* currentTile;
  for (int i = 0; i < amountOfTiles; i++) {
    if (xAxis == tiles[i]->getXCoordinate() && yAxis == tiles[i]->getYCoordinate()) { //check if there is a tile on x and y coordinates.
      currentTile = tiles[i];
    }
  }

  //check if robot has been on the same tile, on the same orientation
  int sameOrientationAndTile = -1;
  for (int j = 0; j < amountOfTiles; j++) {
    Tile* t = tiles[j];
    int robotOrientationOnTile = robotOrientation[j];
    //    TilePackage tp = tilePackages[j];
    if (t->getXCoordinate() == currentTile->getXCoordinate() && t->getYCoordinate() == currentTile->getYCoordinate() && robotOrientationOnTile == orientation) {
      sameOrientationAndTile++;
    }
  }

  switch (orientation) {
    case 0: //orientation = North
      if (sameOrientationAndTile == 0) {
        if (currentTile->getOptionEast()) {
          junction = 3;
        } else if (currentTile->getOptionNorth()) {
          junction = 2;
        } else if (currentTile->getOptionWest()) {
          junction = 1;
        } else {
          junction = 4;
        }
      } else if (sameOrientationAndTile == 1) {
        if (currentTile->getOptionEast()) {
          if (currentTile->getOptionNorth()) {
            junction = 2;
          } else if (currentTile->getOptionWest()) {
            junction = 1;
          } else {
            junction = 3;
          }
        } else if (currentTile->getOptionNorth()) {
          if (currentTile->getOptionWest()) {
            junction = 1;
          } else {
            junction = 2;
          }
        } else if (currentTile->getOptionWest()) {
          junction = 1;
        } else {
          junction = 4;
        }
      } else if (sameOrientationAndTile == 2) {
        if (currentTile->getOptionEast()) {
          if (currentTile->getOptionNorth()) {
            if (currentTile->getOptionWest()) {
              junction = 1;
            } else {
              junction = 3;
            }
          } else {
            junction = 3;
          }
        } else if (currentTile->getOptionNorth()) {
          junction = 2;
        } else if (currentTile->getOptionWest()) {
          junction = 1;
        } else {
          junction = 4;
        }
      }
      break;
    case 1: //orientation = East
      if (sameOrientationAndTile == 0) {
        if (currentTile->getOptionSouth()) {
          junction = 3;
        } else if (currentTile->getOptionEast()) {
          junction = 2;
        } else if (currentTile->getOptionNorth()) {
          junction = 1;
        } else {
          junction = 4;
        }
      } else if (sameOrientationAndTile == 1) {
        if (currentTile->getOptionSouth()) {
          if (currentTile->getOptionEast()) {
            junction = 2;
          } else if (currentTile->getOptionNorth()) {
            junction = 1;
          } else {
            junction = 3;
          }
        } else if (currentTile->getOptionEast()) {
          if (currentTile->getOptionNorth()) {
            junction = 1;
          } else {
            junction = 2;
          }
        } else if (currentTile->getOptionNorth()) {
          junction = 1;
        } else {
          junction = 4;
        }
      } else if (sameOrientationAndTile == 2) {
        if (currentTile->getOptionSouth()) {
          if (currentTile->getOptionEast()) {
            if (currentTile->getOptionNorth()) {
              junction = 1;
            } else {
              junction = 3;
            }
          } else {
            junction = 3;
          }
        } else if (currentTile->getOptionEast()) {
          junction = 2;
        } else if (currentTile->getOptionNorth()) {
          junction = 1;
        } else {
          junction = 4;
        }
      }
      break;
    case 2:
      if (sameOrientationAndTile == 0) {
        if (currentTile->getOptionWest()) {
          junction = 3;
        } else if (currentTile->getOptionSouth()) {
          junction = 2;
        } else if (currentTile->getOptionEast()) {
          junction = 1;
        } else {
          junction = 4;
        }
      } else if (sameOrientationAndTile == 1) {
        if (currentTile->getOptionWest()) {
          if (currentTile->getOptionSouth()) {
            junction = 2;
          } else if (currentTile->getOptionEast()) {
            junction = 1;
          } else {
            junction = 3;
          }
        } else if (currentTile->getOptionSouth()) {
          if (currentTile->getOptionEast()) {
            junction = 1;
          } else {
            junction = 2;
          }
        } else if (currentTile->getOptionEast()) {
          junction = 1;
        } else {
          junction = 4;
        }
      } else if (sameOrientationAndTile == 2) {
        if (currentTile->getOptionWest()) {
          if (currentTile->getOptionSouth()) {
            if (currentTile->getOptionEast()) {
              junction = 1;
            } else {
              junction = 3;
            }
          } else {
            junction = 3;
          }
        } else if (currentTile->getOptionSouth()) {
          junction = 2;
        } else if (currentTile->getOptionEast()) {
          junction = 1;
        } else {
          junction = 4;
        }
      }
      break;
    case 3:
      if (sameOrientationAndTile == 0) {
        if (currentTile->getOptionNorth()) {
          junction = 3;
        } else if (currentTile->getOptionWest()) {
          junction = 2;
        } else if (currentTile->getOptionSouth()) {
          junction = 1;
        } else {
          junction = 4;
        }
      } else if (sameOrientationAndTile == 1) {
        if (currentTile->getOptionNorth()) {
          if (currentTile->getOptionWest()) {
            junction = 2;
          } else if (currentTile->getOptionSouth()) {
            junction = 1;
          } else {
            junction = 3;
          }
        } else if (currentTile->getOptionWest()) {
          if (currentTile->getOptionSouth()) {
            junction = 1;
          } else {
            junction = 2;
          }
        } else if (currentTile->getOptionSouth()) {
          junction = 1;
        } else {
          junction = 4;
        }
      } else if (sameOrientationAndTile == 2) {
        if (currentTile->getOptionNorth()) {
          if (currentTile->getOptionWest()) {
            if (currentTile->getOptionSouth()) {
              junction = 1;
            } else {
              junction = 3;
            }
          } else {
            junction = 3;
          }
        } else if (currentTile->getOptionWest()) {
          junction = 2;
        } else if (currentTile->getOptionSouth()) {
          junction = 1;
        } else {
          junction = 4;
        }
      }
      break;
  }
}

//check for possible direction based on the type of tile, orientation of the tile and the orientation of the robot.
boolean checkDirectionPossibility(int tileType, int tileOrientation, int moveDirection) {
  if (tileOrientation > 3) {
    tileOrientation = tileOrientation - 4;
  }
  switch (moveDirection) {
    case 0: //north
      if ((tileType == 0 && tileOrientation == 2) || (tileType == 2 && tileOrientation != 1) || (tileType == 1 && (tileOrientation != 0 && tileOrientation != 1)) ||  tileType == 3) {
        return true;
      }
      break;
    case 1: //east
      if ((tileType == 0 && tileOrientation == 3) || (tileType == 2 && tileOrientation != 2) || (tileType == 1 && (tileOrientation != 1 && tileOrientation != 2)) || tileType == 3) {
        return true;
      }
      break;
    case 2: //south
      if ((tileType == 0 && tileOrientation == 0) || (tileType == 2 && tileOrientation != 3) || (tileType == 1 && (tileOrientation != 2 && tileOrientation != 3)) || tileType == 3) {
        return true;
      }
      break;
    case 3: //west
      if ((tileType == 0 && tileOrientation == 1) || (tileType == 2 && tileOrientation != 0) || (tileType == 1 && (tileOrientation != 0 && tileOrientation != 3)) ||  tileType == 3) {
        return true;
      }
      break;
  }
  return false;
}

bool done = false;

void loop() {
  int sensor1 = readSensor(1);
  int sensor2 = readSensor(2);
  int sensor3 = readSensor(3);

  if(sensor2 > blackThreshold && sensor1 < blackThreshold && sensor3 < blackThreshold){
    moveStraight();
  } else if(sensor1 > blackThreshold){
    moveWideLeft();
  }else if(sensor3 > blackThreshold){
    moveWideRight();
  }

 //if (engineStatus != 0) {
    //if (junction == 0) {
//      if (sensor1 > blackThreshold && sensor3 > blackThreshold) {
//        moveWideLeft();
//      } else if (sensor1 < blackThreshold && sensor2 < blackThreshold && sensor3 < blackThreshold) {
//        moveStraight();
//      } else if (sensor1 > blackThreshold) {
//        moveWideLeft();
//      } else if (sensor3 > blackThreshold) {
//        moveWideRight();
//      } else {
//        moveStraight();
//      }
    //}

    /*if (junction == 1) {
      receiveMessage();
      orientation--;
      if (orientation < 0) {
        orientation = orientation + 4;
      }
      setNewLocation();
      moveWideLeft();
      delay(500);
      while (readSensor(1) < blackThreshold) {
        //keep turning left until sensor 1 detects black.
      }
      findLine();
    }

    if (junction == 2) {
      receiveMessage();
      setNewLocation();
      moveStraight();
      delay(250);
      findLine();
    }

    if (junction == 3) {
      receiveMessage();
      orientation++;
      if (orientation > 3) {
        orientation = orientation - 4;
      }
      setNewLocation();
      moveWideRight();
      delay(500);
      while (readSensor(3) < blackThreshold) {
        //keep turning right until sensor 1 detects black.
      }
      findLine();
    }

    if (junction == 4) {
      receiveMessage();
      orientation = orientation - 2;
      if (orientation < 0) {
        orientation = orientation + 4;
      }
      setNewLocation();
      moveLeft();
      delay(1000);
      while (readSensor(1) < blackThreshold) {
        //keep turning left until sensor 1 detects black.
      }
      findLine();
    }
  } else {
    receiveMessage();
  }*/
}
