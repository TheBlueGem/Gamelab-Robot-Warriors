#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "Tile.h"

Tile* tiles[50];
int robotOrientation[50];

int amountOfTiles = 0;
int amountOfPackages = 0;

RF24 myRadio (9, 10);
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

struct package {
  String position;
  int sensor1;
  int sensor2;
  int sensor3;  
  //  char text[20];
};

int packageId = 0;

typedef struct package Package;

void setup()
{
  Serial.begin(57600);
  //delay(1000);
  myRadio.begin();
  //myRadio.setDataRate( RF24_250KBPS ) ;
  myRadio.openReadingPipe(1,pipes[1]);
  myRadio.startListening();
  Serial.print("setup ");
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

void receiveMessage() {
  
}

void loop(void) {  
  if ( Serial.available() )
  {
    char c = toupper(Serial.read());
    if ( c == 'T' )
    {
      printf("*** CHANGING TO TRANSMIT ROLE -- PRESS 'R' TO SWITCH BACK\n\r");

      // Become the primary transmitter (ping out)
    
      myRadio.openWritingPipe(pipes[0]);
      myRadio.openReadingPipe(1,pipes[1]);
    }
  }

  //receiveMessage(); 
  if (myRadio.available()) {
    Serial.println("radio is beschickbaar ");
    Package data;
    unsigned long time;
    
    while (myRadio.available()) {
      myRadio.read(&data, sizeof(data) ); 
      delay(20);
      }
      Serial.println(data.position);
      Serial.println(data.sensor1);
      Serial.println(data.sensor2);
      Serial.println(data.sensor3);

      // First, stop listening so we can talk
      myRadio.stopListening();

      // Send the final one back.
      myRadio.write( &time, sizeof(unsigned long) );
     Serial.println("sent shit");

      // Now, resume listening so we catch the next packets.
      myRadio.startListening();
  }
}
