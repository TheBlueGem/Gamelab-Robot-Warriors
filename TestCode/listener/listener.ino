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
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };


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
  myRadio.openReadingPipe(1, pipes[1]);
  myRadio.startListening();
  Serial.println("setup ");
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

void printMessage(unsigned long id) {
  switch (id)
  {
    case -1:
      Serial.println("Unrecognized log message");
      break;
    case 0: // arrive from right side of the corner.
      Serial.println("Arrived from the right side of a corner...");
      break;
    case 1: // arrive from left side of the corner.
      Serial.println("Arrived from the left side of a corner...");
      break;
    case 2: // arrive from bottom side of a T tile.
      Serial.println("Arrived fron the bottom side of a T tile...");
      break;
    case 3: // arrive from left side of the T.
      Serial.println("Arrived fron the left side of a T tile...");
      break;
    case 4: // arrive from right side of the T.
      Serial.println("Arrived fron the right side of a T tile...");
      break;
    case 5:
      Serial.println("Arrived on an intersection...");
      break;
    case 6:
      Serial.println("Finished!");
      break;
    case 7:
      Serial.println("Dead end...");
      break;
    case 8:
      Serial.println("Moving Straight...");
      break;
    case 9:
      Serial.println("Moving left...");
      break;
    case 10:
      Serial.println("Moving right...");
      break;
    case 11:
      Serial.println("Moving left wide...");
      break;
    case 12:
      Serial.println("Moving right wide...");
      break;
  }

}

int expectedId = 1;
unsigned long lastMessageId = -1;
void loop(void) {

  //receiveMessage();
  if (myRadio.available()) {
    unsigned long id;
    myRadio.read(&id, sizeof(unsigned long));

      printMessage(id);
  
    /* Package data1 = {1, 90, 70, 30};
       Package data;
       unsigned long time = 80;
       if(!expectedId == data.id){
         expectedId = data.id;
       }
       Serial.println("radio is beschickbaar ");
       myRadio.read(&data, sizeof(Package));
       if(data.id == expectedId)
       {
       char buf[16];
       dtostrf(data.sensor1, 2, 0, buf);
        char buf1[16];
       dtostrf(data.sensor2, 2, 0, buf1);
        char buf2[16];
       dtostrf(data.sensor3, 2, 0, buf2);
       // Serial.println(data.position);
       printf("Sensor 1 %s \n\r", buf);
       printf("Sensor 2 %s \n\r", buf1);
       printf("Sensor 3 %s \n\r", buf2);
       expectedId++;
       }
              // Serial.println(time);
       delay(20);



      // First, stop listening so we can talk
      myRadio.stopListening();

      // Send the final one back.
      //  int sendback = expectedId + 1;
      myRadio.write( &expectedId, sizeof(expectedId) );
      Serial.println("sent shit");
      Serial.println(expectedId);
 */
      // Now, resume listening so we catch the next packets.
      myRadio.startListening();
   

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
