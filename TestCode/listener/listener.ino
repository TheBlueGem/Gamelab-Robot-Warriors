{\rtf1\ansi\deff0\nouicompat{\fonttbl{\f0\fnil\fcharset0 Calibri;}}
{\*\generator Riched20 10.0.10586}\viewkind4\uc1 
\pard\sa200\sl276\slmult1\f0\fs22\lang19 #include <SPI.h>\par
#include "nRF24L01.h"\par
#include "RF24.h"\par
#include "Tile.h"\par
\par
Tile* tiles[50];\par
int robotOrientation[50];\par
\par
int amountOfTiles = 0;\par
int amountOfPackages = 0;\par
\par
RF24 myRadio (9, 10);\par
const uint64_t pipes[2] = \{ 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL \};\par
\par
struct package \{\par
  int id;\par
  int tile;\par
  int tileOrientation;\par
  int xCoordinate;\par
  int yCoordinate;\par
  int robotOrientation;\par
  //  char text[20];\par
\};\par
\par
int packageId = 0;\par
\par
typedef struct package Package;\par
\par
void setup()\par
\{\par
  Serial.begin(57600);\par
  //delay(1000);\par
  myRadio.begin();\par
  //myRadio.setDataRate( RF24_250KBPS ) ;\par
  myRadio.openReadingPipe(1,pipes[1]);\par
  myRadio.startListening();\par
  Serial.print("setup ");\par
\}\par
\par
void addTile(int tileType, int tileOrientation, int x, int y, int rOrientation) \{\par
  Tile* newTile = new Tile(tileType, tileOrientation, x, y);\par
  //sendMessage(newTile->getType(), newTile->getOrientation(), x, y, orientation);\par
  boolean tileKnown = false;\par
  for (int i = 0; i < amountOfTiles; i++) \{\par
    if (x == tiles[i]->getXCoordinate() && y == tiles[i]->getYCoordinate()) \{ //check if there is a tile on x and y coordinates.\par
      newTile = tiles[i];\par
      tileKnown = true;\par
    \}\par
  \}\par
  if (!tileKnown) \{\par
    for (int i = 0; i < amountOfTiles; i++) \{\par
      Tile* checkingTile = tiles[i];\par
      int x2 = checkingTile->getXCoordinate();\par
      int y2 = checkingTile->getYCoordinate();\par
      if (x == x2 && y + 1 == y2) \{ //CheckingTile is north of newTile\par
        newTile->setNorth(checkingTile);\par
        checkingTile->setSouth(newTile);\par
      \}\par
      if (x + 1 == x2 && y == y2) \{ //CheckingTile is east of newTile\par
        newTile->setEast(checkingTile);\par
        checkingTile->setWest(newTile);\par
      \}\par
      if (x == x2 && y - 1 == y2) \{ //CheckingTile is south of newTile\par
        newTile->setSouth(checkingTile);\par
        checkingTile->setNorth(newTile);\par
      \}\par
      if (x - 1 == x2 && y == y2) \{ //CheckingTile is west of newTile\par
        newTile->setWest(checkingTile);\par
        checkingTile->setEast(newTile);\par
      \}\par
    \}\par
\par
    bool north = checkDirectionPossibility(tileType, tileOrientation, 0);\par
    bool east = checkDirectionPossibility(tileType, tileOrientation, 1);\par
    bool south = checkDirectionPossibility(tileType, tileOrientation, 2);\par
    bool west = checkDirectionPossibility(tileType, tileOrientation, 3);\par
\par
    newTile->setOptionNorth(north);\par
    newTile->setOptionSouth(south);\par
    newTile->setOptionEast(east);\par
    newTile->setOptionWest(west);\par
\par
    //sendMessage(packageId, x, y, (String) ammountOfTiles);\par
  \}\par
  tiles[amountOfTiles] = newTile;\par
  robotOrientation[amountOfTiles] = rOrientation;\par
  amountOfTiles++;\par
\}\par
\par
//check for possible direction based on the type of tile, orientation of the tile and the orientation of the robot.\par
boolean checkDirectionPossibility(int tileType, int tileOrientation, int moveDirection) \{\par
  if (tileOrientation > 3) \{\par
    tileOrientation = tileOrientation - 4;\par
  \}\par
  switch (moveDirection) \{\par
    case 0: //north\par
      if ((tileType == 0 && tileOrientation == 2) || (tileType == 2 && tileOrientation != 1) || (tileType == 1 && (tileOrientation != 0 && tileOrientation != 1)) ||  tileType == 3) \{\par
        return true;\par
      \}\par
      break;\par
    case 1: //east\par
      if ((tileType == 0 && tileOrientation == 3) || (tileType == 2 && tileOrientation != 2) || (tileType == 1 && (tileOrientation != 1 && tileOrientation != 2)) || tileType == 3) \{\par
        return true;\par
      \}\par
      break;\par
    case 2: //south\par
      if ((tileType == 0 && tileOrientation == 0) || (tileType == 2 && tileOrientation != 3) || (tileType == 1 && (tileOrientation != 2 && tileOrientation != 3)) || tileType == 3) \{\par
        return true;\par
      \}\par
      break;\par
    case 3: //west\par
      if ((tileType == 0 && tileOrientation == 1) || (tileType == 2 && tileOrientation != 0) || (tileType == 1 && (tileOrientation != 0 && tileOrientation != 3)) ||  tileType == 3) \{\par
        return true;\par
      \}\par
      break;\par
  \}\par
  return false;\par
\}\par
\par
void receiveMessage() \{\par
  \par
\}\par
\par
void loop(void) \{\par
  myRadio.openWritingPipe(pipes[1]);\par
  myRadio.openReadingPipe(1,pipes[0]);\par
  //receiveMessage(); \par
  if (myRadio.available()) \{\par
    Serial.print("radio is beschickbaar ");\par
    Package data;\par
    unsigned long time;\par
    \par
    while (myRadio.available()) \{\par
      myRadio.read(&time, sizeof(unsigned long) );\par
       Serial.println(time);\par
    \par
    /*Serial.print("R: ");\par
    Serial.print("id: ");\par
    Serial.print(data.id);\par
    Serial.print("Tile: ");\par
    Serial.print(data.tile);\par
    Serial.print(", tOrientation: ");\par
    Serial.print(data.tileOrientation);\par
    Serial.print(", x: ");\par
    Serial.print(data.xCoordinate);\par
    Serial.print(", y: ");\par
    Serial.print(data.yCoordinate);\par
    Serial.print(", rOrientation: ");\par
    Serial.println(data.robotOrientation);\par
    */\par
     delay(20);\par
      \}\par
\par
      // First, stop listening so we can talk\par
      myRadio.stopListening();\par
\par
      // Send the final one back.\par
      myRadio.write( &time, sizeof(unsigned long) );\par
     Serial.println("sent shit");\par
\par
      // Now, resume listening so we catch the next packets.\par
      myRadio.startListening();\par
\par
  \}\par
  \par
\par
  \par
\}\par
}
 