#include "Tile.h"

Tile::Tile(int orientation, int type, int xCoordinate, int yCoordinate){
	this->orientation = orientation;
	this->type = type;
	this->xCoordinate = xCoordinate;
  this->yCoordinate = yCoordinate;
}

void Tile::setNorth(Tile* tile){
  this->north = tile;
}

void Tile::setEast(Tile* tile){
  this->east = tile;
}

void Tile::setSouth(Tile* tile){
  this->south = tile;
}

void Tile::setWest(Tile* tile){
  this->west = tile;
}

Tile* Tile::getNorth(){
  return this->north;
}

Tile* Tile::getEast(){
  return this->north;
}

Tile* Tile::getSouth(){
  return this->north;
}

Tile* Tile::getWest(){
  return this->north;
}

int Tile::getOrientation(){
  return this->orientation;
}

void Tile::setOrientation(int orientation){
  this->orientation = orientation;
}

int Tile::getType(){
  return this->type;
}

void Tile::setType(int type){
  this->type = type;
}

int Tile::getXCoordinate(){
  return this->xCoordinate;
}

void Tile::setXCoordinate(int xCoordinate){
  this->xCoordinate = xCoordinate;
}

int Tile::getYCoordinate(){
  return this->yCoordinate;
}

void Tile::setYCoordinate(int yCoordinate){
  this->yCoordinate = yCoordinate;
}

bool Tile::getOptionNorth(){
  return this->optionNorth;
}

void Tile::setOptionNorth(bool optionNorth){
  this->optionNorth = optionNorth;
}

bool Tile::getOptionSouth(){
  return this->optionSouth;
}

void Tile::setOptionSouth(bool optionSouth){
  this->optionSouth = optionSouth;
}

bool Tile::getOptionEast(){
  return this->optionEast;
}

void Tile::setOptionEast(bool optionEast){
  this->optionEast = optionEast;
}

bool Tile::getOptionWest(){
  return this->optionWest;
}

void Tile::setOptionWest(bool optionWest){
  this->optionWest = optionWest;
}














