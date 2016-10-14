#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

RF24 myRadio (9, 10);
const uint64_t pipes[4] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL, 0xF0F0F0F0A1LL, 0xF0F0F0F0A2LL };

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

void printMessage(unsigned long id) {
  switch (id)
  {
    case 0:
      Serial.println("Unrecognized log message");
      //Serial.println(o);
    
      break;
    case 1:
      Serial.println("Dead end Not Possible...");
      //Serial.println(o);   
      break;
    case 2:
      Serial.println("Dead end Possible...");
      //Serial.println(o);      
      break;
    case 3:
      Serial.println("Checking...");
      //Serial.println(o);    
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
    case 8:
      Serial.println("Dead end check activated...");
      break;
    case 9:
      Serial.println("Dead end check deactivated...");
      break;    
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
      Serial.println("Moving West...");
      break;
  }

}

unsigned long lastMessageId = 0;

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
        Serial.print(" Last Message Time (millis): ");
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

