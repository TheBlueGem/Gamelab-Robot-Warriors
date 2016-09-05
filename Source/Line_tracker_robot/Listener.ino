#include <SPI.h>
#include "RF24.h"
#include <Tile.h>

Tile* tiles[50];
int robotOrientation[50];

int amountOfTiles = 0;
int amountOfPackages = 0;

RF24 myRadio (2, 3);
byte addresses[][6] = {"0"};

struct package {
  int id;
  int tile;
  int tileOrientation;
  int xCoordinate;
  int yCoordinate;
  int robotOrientation;
  //  char text[20];
};

int packageId = 0;

typedef struct package Package;

void setup()
{
  Serial.begin(9600);
  delay(1000);
  myRadio.begin();
  myRadio.setChannel(115);
  myRadio.setPALevel(RF24_PA_MAX);
  myRadio.setDataRate( RF24_250KBPS ) ;
  myRadio.openReadingPipe(1, addresses[0]);
  myRadio.startListening();
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
  if (myRadio.available()) {
    Package data;
    while (myRadio.available()) {
      myRadio.read(&data, sizeof(data) );
    }
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
  }
}

void loop() {
  receiveMessage();
}
