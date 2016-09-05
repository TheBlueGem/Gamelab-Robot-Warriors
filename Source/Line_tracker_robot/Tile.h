#ifndef Tile
#define Tile

class Tile
{
	public:
	Tile(int, int, int, int);
	void setNorth(Tile*);
	void setEast(Tile*);
	void setSouth(Tile*);
	void setWest(Tile*);
	
	Tile getNorth();
	Tile getEast();
	Tile getSouth();
	Tile getWest();
	
	int getOrientation();
	void setOrientation();
	
	int getType();
	void setType(int);
	
	int getXCoordinate();
	void setXCoordinate(int);
	
	int getYCoordinate();
	void setYCoordinate(int);
	
	bool getOptionNorth();
	bool getOptionSouth();
	bool getOptionEast();
	bool getOptionWest();
	
	void setOptionNorth(bool);
	void setOptionSouth(bool);
	void setOptionEast(bool);
	void setOptionWest(bool);
	
}