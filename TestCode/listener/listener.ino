#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "Tile.h"
#include "printf.h"

Tile* tiles[50];
int robotOrientation[50];

int amountOfTiles = 0;
int amountOfPackages = 0;

RF24 myRadio (9, 10);
const uint64_t pipes[4] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL, 0xF0F0F0F0A1LL, 0xF0F0F0F0A2LL };


int msg[1];
int resp[1];
String theMessage = "";

struct package {
  unsigned long id;
  float sensor1;
  float sensor2;
  float sensor3;
  //  char text[20];
};

int packageId = 0;

typedef struct package Package;

void setup()
{
  Serial.begin(9600);
  printf_begin();
  myRadio.begin();
  myRadio.openWritingPipe(pipes[0]);
  myRadio.openReadingPipe(1, pipes[1]);
  myRadio.stopListening();
  myRadio.startListening();
  Serial.println("Setup");
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

int o = 1;
void printMessage(unsigned long id) {
  switch (id)
  {
    case -1:
      Serial.println("Unrecognized log message");
      break;
    case 0:
      Serial.print("Dead end...");
      //Serial.println(o);
      o++;
      break;
    case 1:
      Serial.print("Left corner...");
      //Serial.println(o);
      o++;
      break;
    case 2:
      Serial.print("Right corner...");
      //Serial.println(o);
      o++;
      break;
    case 3:
      Serial.print("Arrived fron the left side of a T tile...");
      //Serial.println(o);
      o++;
      break;
    case 4:
      Serial.println("Arrived fron the middle side of a T tile...");
      break;
    case 5:
      Serial.println("Arrived fron the right side of a T tile...");
      break;
    case 6:
      Serial.println("Straight tile...");
      break;
    case 7:
      Serial.println("Dead end...");
      break;
    /*case 8:
      Serial.println("Moving Straight...");
      break;
    case 9:
      Serial.println("Moving left...");
      break;*/
    case 10:
      Serial.println("Moving North...");
      break;
    case 11:
      Serial.println("Moving East...");
      break;
    case 12:
      Serial.println("Moving South...");
      break;
    case 13:
      Serial.print("Moving West...");
      break;
  }

}

unsigned long lastMessageId = -1;

unsigned long lastMessageTime = 0;

void loop(void) {
  //receiveMessage();
  if (myRadio.available()) {
    unsigned long receivedId;
    bool hasRead = myRadio.read(&receivedId, sizeof(unsigned long));
    if (hasRead) {      
      if (receivedId != lastMessageId || (millis() - lastMessageTime) > 500) { 

        Serial.print("Last message ID: ");
        Serial.print(lastMessageId);
        Serial.print("Last Message Time (millis): ");
        Serial.print(lastMessageTime);
        Serial.println();
        printMessage(receivedId);
        myRadio.stopListening();
        /*unsigned long sentId = 1;
        //delay(20);
        
        bool confirmed = myRadio.write(&sentId, sizeof(unsigned long));
        if (confirmed) {
          Serial.println("Sent confirmation");
          lastMessageTime = millis(); 
        }*/
        lastMessageId = receivedId;
        lastMessageTime = millis(); 
        myRadio.startListening();
      }
    }
  }

  if ( Serial.available() )
  {
    char c = toupper(Serial.read());
    if ( c == 'T' )
    {
      Serial.println("CHANGING TO TRANSMIT ROLE -- PRESS 'R' TO SWITCH BACK");

      // Become the primary transmitter (ping out)

      myRadio.openWritingPipe(pipes[0]);
      myRadio.openReadingPipe(1, pipes[1]);
    }
  }
}
