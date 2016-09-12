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

RF24 myRadio(9, 10);

// Topology
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL }; // Radio pipe addresses for the 2 nodes to communicate.

struct package
{
	int id;
	int tile;
	int tileOrientation;
	int xPosition;
	int yPosition;
	int robotOrientation;
};

typedef struct package Package;

int orientation = 0; // 0 = north, 1 = east, 2 = south, 3 = west
int xPosition = 0;
int yPosition = 0;

//int followLineDifference = 3;
int blackThreshold = 60;

int midleftSensorWhiteValue;
int middleSensorWhiteValue;
int midrightSensorWhiteValue;
int leftSensorWhiteValue;
int rightSensorWhiteValue;
boolean logging = true;
int movementMode; // 0 = engines off, 1 = straight, 2 = reverse, 3 = left, 4 = wideLeft, 5 = right, 6 = wideRight
int junction = 0; //followLine = 0, left = 1, straight = 2, right = 3, reverse = 4

void Motor1(int pwm, boolean reverse)
{
	analogWrite(6, pwm); //set pwm control, 0 for stop, and 255 for maximum speed
	if (reverse)
	{
		digitalWrite(7, HIGH);
	}
	else
	{
		digitalWrite(7, LOW);
	}
}

void Motor2(int pwm, boolean reverse)
{
	analogWrite(5, pwm);
	if (reverse)
	{
		digitalWrite(4, HIGH);
	}
	else
	{
		digitalWrite(4, LOW);
	}
}

void setup()
{
	for (int i = 4; i <= 7; i++) //For Arduino Motor Shield
		pinMode(i, OUTPUT);  //set pin 4,5,6,7 to output mode

	Serial.begin(9600);
	printf_begin();

	myRadio.begin();
	myRadio.openWritingPipe(pipes[1]);        // The radios need two pipes to communicate. One reading pipe and one writing pipe.
	myRadio.openReadingPipe(1, pipes[0]);
	myRadio.startListening();                 // Switch to reading pipe

	int midleftSensor = 0;
	int middleSensor = 0;
	int midrightSensor = 0;
	int leftSensor = 0;
	int rightSensor = 0;

	for (int i = 0; i < 10; i++)
	{
		leftSensor = analogRead(A0);
		midleftSensor = analogRead(A1);
		middleSensor = analogRead(A2);
		midrightSensor = analogRead(A3);
		rightSensor = analogRead(A4);

		leftSensorWhiteValue += leftSensor;
		middleSensorWhiteValue += middleSensor;
		rightSensorWhiteValue += rightSensor;
		midleftSensorWhiteValue += midleftSensor;
		midrightSensorWhiteValue += midrightSensor;

	}

	leftSensorWhiteValue = leftSensorWhiteValue / 10;
	middleSensorWhiteValue = middleSensorWhiteValue / 10;
	rightSensorWhiteValue = rightSensorWhiteValue / 10;
	midleftSensorWhiteValue = midleftSensorWhiteValue / 10;
	midrightSensorWhiteValue = midrightSensorWhiteValue / 10;

	printf("Values left: %d mid left: %d middle: %d mid right: %d right: %d", leftSensorWhiteValue, midleftSensorWhiteValue, middleSensorWhiteValue, midrightSensorWhiteValue, rightSensorWhiteValue);

	Motor1(0, false);
	Motor2(0, true);
	movementMode = 0;
	Serial.println("Setup Done");
	findLine();
}

void findLine()
{
	Serial.println("Finding line....");
	unsigned long startTime = millis();
	unsigned long searchTime = 0;

	int left = 0;
	int middle = 0;
	int right = 0;
	int midleft = 0;
	int midright = 0;
	while (midleft < blackThreshold)
	{
		left = readSensor(1);
		midleft = readSensor(2);
		middle = readSensor(3);
		midright = readSensor(4);
		right = readSensor(5);
		printf("Values midleft: %d middle: %d midright: %d\n", midleft, middle, midright);
		if (searchTime > 5000 && searchTime < 6000)
		{
			moveStraight();
			searchTime = millis() - startTime;
		}
		else if (searchTime > 6000)
		{
			startTime = millis();
			searchTime = 0;
		}
		else
		{
			moveWideLeft();
			searchTime = millis() - startTime;
		}

		//printf("Search search... %d\n", searchTime);
	}

	Serial.println("Line found!");
}

int readSensor(int sensor)
{ //returns a percentage of how black the sensor is. 0 = white; 100 = black.
	double result;
	int detectedValue;
	int range;
	int sensorValue;
	switch (sensor)
	{
	case 1: // read sensor 1.
		sensorValue = analogRead(A0);
		/*if(sensorValue < leftSensorWhiteValue)
		{
		  leftSensorWhiteValue = sensorValue;
		}*/
		detectedValue = sensorValue - leftSensorWhiteValue;
		range = 1024 - leftSensorWhiteValue;
		break;
	case 2: // read sensor 2.
		sensorValue = analogRead(A1);
		/*if(sensorValue < middleSensorWhiteValue)
		{
		  middleSensorWhiteValue = sensorValue;
		}*/
		detectedValue = sensorValue - midleftSensorWhiteValue;
		range = 1024 - midleftSensorWhiteValue;
		break;
	case 3: // read sensor 3.
		sensorValue = analogRead(A2);
		/*if(sensorValue < rightSensorWhiteValue)
		{
		  rightSensorWhiteValue = sensorValue;
		}*/
		detectedValue = sensorValue - middleSensorWhiteValue;
		range = 1024 - middleSensorWhiteValue;
		break;
	case 4: // read sensor 4.
		sensorValue = analogRead(A3);
		/*if(sensorValue < rightSensorWhiteValue)
		{
		rightSensorWhiteValue = sensorValue;
		}*/
		detectedValue = sensorValue - midrightSensorWhiteValue;
		range = 1024 - midrightSensorWhiteValue;
		break;
	case 5: // read sensor 5.
		sensorValue = analogRead(A4);
		/*if(sensorValue < rightSensorWhiteValue)
		{
		rightSensorWhiteValue = sensorValue;
		}*/
		detectedValue = sensorValue - rightSensorWhiteValue;
		range = 1024 - rightSensorWhiteValue;
		break;
	}

	result = ((double)detectedValue / (double)range) * 100.0;

	return (int)result;
}

void setNewLocation()
{
	switch (orientation)
	{
	case 0:
		yPosition++;
		break;
	case 1:
		xPosition++;
		break;
	case 2:
		yPosition--;
		break;
	case 3:
		xPosition--;
		break;
	}
}

void moveStraight()
{
	if (movementMode != 1)
	{
		// Motor1(255, false);
	   //  Motor2(255, true);
		movementMode = 1;
	}
}

void moveLeft()
{
	if (movementMode != 3)
	{
		//   Motor1(255, true);
		 //  Motor2(255, true);
		sendLogMessage(9);
		movementMode = 3;
	}
}

void moveWideLeft()
{
	if (movementMode != 4)
	{
		//   Motor1(0, true);
		//   Motor2(255, true);
		sendLogMessage(11);
		movementMode = 3;
	}
}

void moveRight()
{
	if (movementMode != 5)
	{
		//   Motor1(255, false);
		//   Motor2(255, false);
		sendLogMessage(10);
		movementMode = 4;
	}
}

void moveWideRight()
{
	if (movementMode != 6)
	{
		//   Motor1(255, false);
		//   Motor2(0, false);
		sendLogMessage(12);

		movementMode = 4;
	}
}

void turnOff()
{
	Motor1(0, false);
	Motor2(0, true);
	if (logging && movementMode != 0)
	{
		Serial.println("Engines Off");
	}

	movementMode = 0;
}

/*void addTile(int tileType, int tileOrientation, int x, int y, int robotOrientation)
{
  Tile* newTile = new Tile(tileType, tileOrientation, x, y);
  //sendMessage(newTile->getType(), newTile->getOrientation(), x, y, orientation);
  boolean tileKnown = false;
  for(int i = 0; i < amountOfTiles; i++)
  {
	if(x == tiles[i]->getXCoordinate() && y == tiles[i]->getYCoordinate())
	{ //check if there is a tile on x and y coordinates.
	  newTile = tiles[i];
	  tileKnown = true;
	}
  }
  if(!tileKnown)
  {
	for(int i = 0; i < amountOfTiles; i++)
	{
	  Tile* checkingTile = tiles[i];
	  int x2 = checkingTile->getXCoordinate();
	  int y2 = checkingTile->getYCoordinate();
	  if(x == x2 && y + 1 == y2)
	  { //CheckingTile is north of newTile
		newTile->setNorth(checkingTile);
		checkingTile->setSouth(newTile);
	  }
	  if(x + 1 == x2 && y == y2)
	  { //CheckingTile is east of newTile
		newTile->setEast(checkingTile);
		checkingTile->setWest(newTile);
	  }
	  if(x == x2 && y - 1 == y2)
	  { //CheckingTile is south of newTile
		newTile->setSouth(checkingTile);
		checkingTile->setNorth(newTile);
	  }
	  if(x - 1 == x2 && y == y2)
	  { //CheckingTile is west of newTile
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

  //tiles[amountOfTiles] = newTile;
  //robotOrientation[amountOfTiles] = rOrientation;
  //amountOfTiles++;
}*/

void detectTile(int left, int midleft, int middle, int midright, int right)
{
	left = readSensor(1);
	midleft = readSensor(2);
	middle = readSensor(3);
	midright = readSensor(4);
	right = readSensor(5);
	//printf("Values left: %d", left);
	printf(" Values midleft: %d", midleft);
	printf(" Values middle: %d", middle);
	printf(" Values midright: %d\n", midright);
	//printf(" Values right: %d\n", right);

	if (midleft > blackThreshold && midright < blackThreshold && middle < blackThreshold)
	{
			readTile(0, 0);
			Motor1(255, true);
			Motor2(255, true);

			Serial.println("Naar links");
		//	printf("Values left: %d", left);
			printf(" Values midleft: %d", midleft);
			printf(" Values middle: %d", middle);
			printf(" Values midright: %d\n", midright);
			//printf(" Values right: %d\n", right);
	
	}
	if (midleft < blackThreshold && midright < blackThreshold && middle > blackThreshold)
	{
		readTile(0, 6);
		Motor1(255, false);
	    Motor2(255, true);
	
		
		Serial.println("Naar voren");
	//	printf("Values left: %d", left);
		printf(" Values midleft: %d", midleft);
		printf(" Values middle: %d", middle);
		printf(" Values midright: %d\n", midright);
	//	printf(" Values right: %d\n", right);
	}

	if (midleft < blackThreshold && midright > blackThreshold && middle < blackThreshold)
	{
		left = readSensor(1);
		middle = readSensor(2);
		right = readSensor(3);
		readTile(2, 1);
		Motor1(255, false);
		Motor2(255, false);
	
		Serial.println("Naar rechts");
	//	printf("Values left: %d", left);
		printf(" Values midleft: %d", midleft);
		printf(" Values middle: %d", middle);
		printf(" Values midright: %d\n", midright);
	//	printf(" Values right: %d\n", right);
	}


	/*  if(left > blackThreshold)
	  {
		if(middle > blackThreshold)
		{
		  if(right > blackThreshold)
		  { //bbb
			readTile(3, 5);
		  }
		  else
		  { //bbw
			readTile(1, 4);
		  }
		}
		else
		{
		  if(right > blackThreshold)
		  { //bwb
			readTile(1, 2);
		  }
		  else
		  { //bww
			readTile(0, 0);
		  }
		}
	  }
	  else
	  {
		if(middle > blackThreshold)
		{
		  if(right > blackThreshold)
		  { //wbb
			readTile(1, 3);
		  }
		  else
		  { //wbw
			readTile(2, 0);
		 }
		}
		else
		{
		  if(right > blackThreshold)
		  { //wwb
			readTile(0, 1);
		  }
		  else
		  { //www

			detectEnd();
		  }
		}
	  }
	*/
}

/**
* Read what kind of Tile we have encountered
*/
void readTile(int type, int caseId)
{
	int arrivalDirection = -1;

	switch (caseId)
	{
	case 0: // Arrived from right side of the corner.
		if (logging)
		{
			sendLogMessage(0);
		}
		arrivalDirection = orientation + 1;
		//addTile(1, orientation + 1, xPosition, yPosition, orientation);
		moveWideLeft();
		break;
	case 1: // Arrived from left side of the corner.
		if (logging)
		{
			sendLogMessage(1);
		}
		arrivalDirection = orientation;
		moveWideRight();
		//addTile(1, orientation, xPosition, yPosition, orientation);    
		break;

	case 2: // Arrived from bottom side of a T tile.
		if (logging)
		{
			sendLogMessage(2);
		}
		moveWideLeft();
		//addTile(2, orientation + 1, xPosition, yPosition, orientation);
		arrivalDirection = orientation + 1;
		break;

	case 3: // Arrived from left side of the T.
		if (logging)
		{
			sendLogMessage(3);
		}
		//addTile(2, orientation, xPosition, yPosition, orientation);
		arrivalDirection = orientation;
		moveWideRight();
		break;

	case 4: // Arrived from right side of the T.
		if (logging)
		{
			sendLogMessage(4);
		}
		//addTile(2, orientation + 2, xPosition, yPosition, orientation);
		arrivalDirection = orientation + 2;
		moveWideLeft();
		break;

	case 5: // Arrived at an intersection.
		if (logging)
		{
			sendLogMessage(5);
		}
		//addTile(3, 0, xPosition, yPosition, orientation);
		arrivalDirection = 0;
		moveWideLeft();
		break;

	case 6:
		if (logging)
		{
			sendLogMessage(8);
		}

		moveStraight();
		break;
	}



	//makeMove();
}

int startTime = millis();
int elapsedTime = 0;

void detectEnd()
{
	//moveStraight();
	if (elapsedTime < 1000)
	{
		elapsedTime = millis() - startTime;
	}
	else
	{
		int left = readSensor(1);
		int middle = readSensor(2);
		int right = readSensor(3);
		if (left > blackThreshold && middle > blackThreshold && right > blackThreshold)
		{
			if (logging)
			{
				sendLogMessage(6);
			}
			//addTile(4, orientation, xPosition, yPosition, orientation);
			//sendMessage(4, orientation, xPosition, yPosition, orientation);
		}
		else
		{
			if (logging)
			{
				sendLogMessage(7);
			}
			//addTile(0, orientation, xPosition, yPosition, orientation);
			//sendMessage(0, orientation, xPosition, yPosition, orientation);
			//makeMove();
		}
		elapsedTime = 0;
	}
}

int lastMessageId = -1;

void logString(int messageId, String message)
{
	if (messageId != lastMessageId) {
		Serial.println(message);
		lastMessageId = messageId;
	}
}

int currentPackageId = 0;

void sendLogMessage(int messageId) {
	if (messageId != lastMessageId) {
		myRadio.stopListening();
		unsigned long id = messageId;
		bool ok = myRadio.write(&id, sizeof(unsigned long));
		if (ok) {
			printf("Message Send:  %d \n", messageId);
		}
		lastMessageId = messageId;
	}

}
void sendMessage(int tile, int tileOrientation, int x, int y, int robotOrientation)
{
	//bool received = false;
	// First, stop listening so we can talk.
	Package data = { currentPackageId, tile, tileOrientation, x, y, robotOrientation };
	//receiveMessage();
	myRadio.stopListening();
	if (!myRadio.write(&data, sizeof(data)))
	{
		Serial.println("failed.");
	}

	myRadio.openWritingPipe(pipes[1]);
	myRadio.openReadingPipe(1, pipes[0]);
	//myRadio.startListening();
	//receiveMessage();
}

void receiveMessage()
{

	Serial.println("startListening");
	Package data;
	while (myRadio.available())
	{
		Serial.println("readPackage");
		myRadio.read(&data, sizeof(data));
		if (data.tile >= 0)
		{
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
			Serial.print(data.xPosition);
			Serial.print(", y: ");
			Serial.print(data.yPosition);
			Serial.print(", rOrientation: ");
			Serial.println(data.robotOrientation);
			//addTile(data.tile, data.tileOrientation, data.xPosition, data.yPosition, data.robotOrientation);
		}
	}

}

//make a choice based on the orientation of the robot, tile type and tile orientation. Always try to go right, if not go straight, if not go left or turn around.
void makeMove()
{
	//get current tile
	Tile* currentTile;
	for (int i = 0; i < amountOfTiles; i++)
	{
		if (xPosition == tiles[i]->getXCoordinate() && yPosition == tiles[i]->getYCoordinate()) //check if there is a tile on the current x and y coordinates.
		{
			currentTile = tiles[i];
		}
	}

	//check if robot has been on this tile with the same orientation
	bool sameOrientationAndTile = false;
	for (int j = 0; j < amountOfTiles; j++)
	{
		Tile* t = tiles[j];
		int robotOrientationOnTile = robotOrientation[j];
		//    TilePackage tp = tilePackages[j];
		if (t->getXCoordinate() == currentTile->getXCoordinate() && t->getYCoordinate() == currentTile->getYCoordinate() && robotOrientationOnTile == orientation)
		{
			sameOrientationAndTile = true;
		}
	}

	switch (orientation)
	{
	case 0: //orientation = North
		if (sameOrientationAndTile)
		{
			if (currentTile->getOptionEast())
			{
				junction = 3;
			}
			else if (currentTile->getOptionNorth())
			{
				junction = 2;
			}
			else if (currentTile->getOptionWest())
			{
				junction = 1;
			}
			else
			{
				junction = 4;
			}
		}
		else
		{
			if (currentTile->getOptionEast())
			{
				if (currentTile->getOptionNorth())
				{
					junction = 2;
				}
				else if (currentTile->getOptionWest())
				{
					junction = 1;
				}
				else
				{
					junction = 3;
				}
			}
			else if (currentTile->getOptionNorth())
			{
				if (currentTile->getOptionWest())
				{
					junction = 1;
				}
				else
				{
					junction = 2;
				}
			}
			else if (currentTile->getOptionWest())
			{
				junction = 1;
			}
			else
			{
				junction = 4;
			}
		}
		/*else if(sameOrientationAndTile == 2)
		{
		if(currentTile->getOptionEast())
		{
		if(currentTile->getOptionNorth())
		{
		if(currentTile->getOptionWest())
		{
		junction = 1;
		}
		else
		{
		junction = 3;
		}
		}
		else
		{
		junction = 3;
		}
		}
		else if(currentTile->getOptionNorth())
		{
		junction = 2;
		}
		else if(currentTile->getOptionWest())
		{
		junction = 1;
		}
		else
		{
		junction = 4;
		}
		}*/
		break;
	case 1: //orientation = East
		if (sameOrientationAndTile)
		{
			if (currentTile->getOptionSouth())
			{
				junction = 3;
			}
			else if (currentTile->getOptionEast())
			{
				junction = 2;
			}
			else if (currentTile->getOptionNorth())
			{
				junction = 1;
			}
			else
			{
				junction = 4;
			}
		}
		else
		{
			if (currentTile->getOptionSouth())
			{
				if (currentTile->getOptionEast())
				{
					junction = 2;
				}
				else if (currentTile->getOptionNorth())
				{
					junction = 1;
				}
				else
				{
					junction = 3;
				}
			}
			else if (currentTile->getOptionEast())
			{
				if (currentTile->getOptionNorth())
				{
					junction = 1;
				}
				else
				{
					junction = 2;
				}
			}
			else if (currentTile->getOptionNorth())
			{
				junction = 1;
			}
			else
			{
				junction = 4;
			}
		}
		/*else if(sameOrientationAndTile == 2)
		{
		if(currentTile->getOptionSouth())
		{
		if(currentTile->getOptionEast())
		{
		if(currentTile->getOptionNorth())
		{
		junction = 1;
		}
		else
		{
		junction = 3;
		}
		}
		else
		{
		junction = 3;
		}
		}
		else if(currentTile->getOptionEast())
		{
		junction = 2;
		}
		else if(currentTile->getOptionNorth())
		{
		junction = 1;
		}
		else
		{
		junction = 4;
		}
		}*/
		break;
	case 2:
		if (sameOrientationAndTile)
		{
			if (currentTile->getOptionWest())
			{
				junction = 3;
			}
			else if (currentTile->getOptionSouth())
			{
				junction = 2;
			}
			else if (currentTile->getOptionEast())
			{
				junction = 1;
			}
			else
			{
				junction = 4;
			}
		}
		else
		{
			if (currentTile->getOptionWest())
			{
				if (currentTile->getOptionSouth())
				{
					junction = 2;
				}
				else if (currentTile->getOptionEast())
				{
					junction = 1;
				}
				else
				{
					junction = 3;
				}
			}
			/*else if(currentTile->getOptionSouth())
			{
			if(currentTile->getOptionEast())
			{
			junction = 1;
			}
			else
			{
			junction = 2;
			}
			}
			else if(currentTile->getOptionEast())
			{
			junction = 1;
			}
			else
			{
			junction = 4;
			}
			}
			else if(sameOrientationAndTile == 2)
			{
			if(currentTile->getOptionWest())
			{
			if(currentTile->getOptionSouth())
			{
			if(currentTile->getOptionEast())
			{
			junction = 1;
			}
			else
			{
			junction = 3;
			}
			}
			else
			{
			junction = 3;
			}
			}
			else if(currentTile->getOptionSouth())
			{
			junction = 2;
			}
			else if(currentTile->getOptionEast())
			{
			junction = 1;
			}
			else
			{
			junction = 4;
			}
			}*/
			break;
	case 3:
		if (sameOrientationAndTile)
		{
			if (currentTile->getOptionNorth())
			{
				junction = 3;
			}
			else if (currentTile->getOptionWest())
			{
				junction = 2;
			}
			else if (currentTile->getOptionSouth())
			{
				junction = 1;
			}
			else
			{
				junction = 4;
			}
		}
		else
		{
			if (currentTile->getOptionNorth())
			{
				if (currentTile->getOptionWest())
				{
					junction = 2;
				}
				else if (currentTile->getOptionSouth())
				{
					junction = 1;
				}
				else
				{
					junction = 3;
				}
			}
			else if (currentTile->getOptionWest())
			{
				if (currentTile->getOptionSouth())
				{
					junction = 1;
				}
				else
				{
					junction = 2;
				}
			}
			else if (currentTile->getOptionSouth())
			{
				junction = 1;
			}
			else
			{
				junction = 4;
			}
		}
		/*else if(sameOrientationAndTile == 2)
		{
		if(currentTile->getOptionNorth())
		{
		if(currentTile->getOptionWest())
		{
		if(currentTile->getOptionSouth())
		{
		junction = 1;
		}
		else
		{
		junction = 3;
		}
		}
		else
		{
		junction = 3;
		}
		}
		else if(currentTile->getOptionWest())
		{
		junction = 2;
		}
		else if(currentTile->getOptionSouth())
		{
		junction = 1;
		}
		else
		{
		junction = 4;
		}
		}*/
		break;
		}
	}
}

//check for possible direction based on the type of tile, orientation of the tile and the orientation of the robot.
/*boolean checkDirectionPossibility(int tileType, int tileOrientation, int moveDirection)
{
  if(tileOrientation > 3)
  {
	tileOrientation = tileOrientation - 4;
  }
  switch(moveDirection)
  {
  case 0: //north
	if((tileType == 0 && tileOrientation == 2) || (tileType == 2 && tileOrientation != 1) || (tileType == 1 && (tileOrientation != 0 && tileOrientation != 1)) || tileType == 3)
	{
	  return true;
	}
	break;
  case 1: //east
	if((tileType == 0 && tileOrientation == 3) || (tileType == 2 && tileOrientation != 2) || (tileType == 1 && (tileOrientation != 1 && tileOrientation != 2)) || tileType == 3)
	{
	  return true;
	}
	break;
  case 2: //south
	if((tileType == 0 && tileOrientation == 0) || (tileType == 2 && tileOrientation != 3) || (tileType == 1 && (tileOrientation != 2 && tileOrientation != 3)) || tileType == 3)
	{
	  return true;
	}
	break;
  case 3: //west
	if((tileType == 0 && tileOrientation == 1) || (tileType == 2 && tileOrientation != 0) || (tileType == 1 && (tileOrientation != 0 && tileOrientation != 3)) || tileType == 3)
	{
	  return true;
	}
	break;
  }
  return false;
}*/

void loop()
{

	int left = readSensor(1);
	int middle = readSensor(3);
	int right = readSensor(5);
	int midleft = readSensor(2);
	int midright = readSensor(4);
	//printf("Values left: %d middle: %d right: %d\n", left, middle, right);

	 detectTile(left, midleft, middle, midright, right);

}


















