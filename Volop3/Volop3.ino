/***************************************************
DFPlayer - A Mini MP3 Player For Arduino
 <https://www.dfrobot.com/product-1121.html>
 
 ***************************************************
 This example shows the basic function of library for DFPlayer.
 
 Created 2016-12-07
 By [Angelo qiao](Angelo.qiao@dfrobot.com)
 
 GNU Lesser General Public License.
 See <http://www.gnu.org/licenses/> for details.
 All above must be included in any redistribution
 ****************************************************/

/***********Notice and Trouble shooting***************
 1.Connection and Diagram can be found here
 <https://www.dfrobot.com/wiki/index.php/DFPlayer_Mini_SKU:DFR0299#Connection_Diagram>
 2.This code is tested on Arduino Uno, Leonardo, Mega boards.
 ****************************************************/

#include "Arduino.h"
#include "string.h"
#include "DFRobotDFPlayerMini.h"

#include "FS.h"
#include "SD.h"
#include "SPI.h"

#if (defined(ARDUINO_AVR_UNO) || defined(ESP8266))   // Using a soft serial port
#include <SoftwareSerial.h>
SoftwareSerial softSerial(/*rx =*/4, /*tx =*/14);
#define FPSerial softSerial
#else
#define FPSerial Serial1
#endif

DFRobotDFPlayerMini myDFPlayer;
File myFile;
void printDetail(uint8_t type, int value);

int curSong = 1;
int busyLine = 13;
int busyState = 0;
int max_file;

const int NUM_SIZE = 4;

void setup()
{
#if (defined ESP32)
  FPSerial.begin(9600, SERIAL_8N1, /*rx =*/4, /*tx =*/14);
#else
  FPSerial.begin(9600);
#endif

  Serial.begin(115200);

  Serial.println();
  Serial.println(F("DFRobot DFPlayer Mini Demo"));
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));
  
  if(!SD.begin(5)){
      Serial.println("Card Mount Failed");
      return;
  }

  uint8_t cardType = SD.cardType();

  if(cardType == CARD_NONE){
      Serial.println("No SD card attached");
      return;
  }

  Serial.print("SD Card Type: ");
  if(cardType == CARD_MMC){
      Serial.println("MMC");
  } else if(cardType == CARD_SD){
      Serial.println("SDSC");
  } else if(cardType == CARD_SDHC){
      Serial.println("SDHC");
  } else {
      Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);

  

  if (!myDFPlayer.begin(FPSerial, /*isACK = */true, /*doReset = */true)) {  //Use serial to communicate with mp3.
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while(true){
      delay(0); // Code to compatible with ESP8266 watch dog.
    }
  }
  Serial.println(F("DFPlayer Mini online."));
  myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD);
  delay(300);
  myDFPlayer.enableDAC();
  myDFPlayer.volume(10);  //Set volume value. From 0 to 30
  curSong = 1;
  myDFPlayer.playMp3Folder(curSong);  //Play the first mp3
  delay(500);

}

unsigned long timer = 100;
bool ready = false;
bool check = true;
int lastVal = 0;
int i = 0;
unsigned char dummy;
char readVals[200];
char title[200];

void loop()
{

  int j = 0;
  
  String testy;
  lastVal = busyState;
  busyState = digitalRead(busyLine);
  
  if (busyState != lastVal){
    Serial.print("BusyLine: ");
    Serial.println(busyState);
  }

  for (int k = 0; k < 200; k++){
    readVals[k] = '\0';
  }
  
  i++;
  
  findNumber(readVals, 200, 679, 20000);
  Serial.println(readVals);
  extractTitle(title, 200, NUM_SIZE, readVals, 200);
  Serial.println(title);

  if ((busyState >= 1) && (i >= 40)) {
    delay(500);
    i = 0;
    ready = false;
    check = true;
    // Serial.print("Finished Playing ");
    // Serial.print(curSong);
    // Serial.print(", Now Playing: ");
    curSong++;
    // Serial.println(curSong);
    //delay(100);
    timer = millis();
    myDFPlayer.playMp3Folder(curSong);  //Play next mp3 every 3 second.
    delay(500);
  }
  
  if (myDFPlayer.available()) {
    printDetail(myDFPlayer.readType(), myDFPlayer.read()); //Print the detail message from DFPlayer to handle different errors and states.
  }
  delay(500);
}

//takes a given number and returns the corresponding complete song string in the buffer
void findNumber(char *buff, int size, int target, int max){
  char tempRead[size];
  char lineNumStr[NUM_SIZE];
  int lineNum;
  int j = 0;

  myFile = SD.open("/demofile.txt");

  if (myFile.available()){

    for (int o = 0; o < max; o++){

      for (int p = 0; p < size; p++){
        tempRead[p] = '\0';
      }
      dummy = myFile.read();
      j = 0;
      while (dummy != '\n'){
        tempRead[j] = (char)dummy;
        dummy = myFile.read();
        j++;
      }

      for (int m = 0; m < NUM_SIZE; m++){
        lineNumStr[m] = tempRead[m];
      }

      lineNum = atoi(lineNumStr);
      //Serial.println(lineNum);

      if (lineNum == target){
        for (int n = 0; n < size; n++){
          buff[n] = tempRead[n];
        }
        myFile.close();
        Serial.println("Number Found!");
        return;
      }
    }
    myFile.close();
    Serial.println("Number does not exist, register unchanged.");
    return;
    //Serial.println(readVals);
    
  } else {
    Serial.println("We are not available");
  }

}

//takes a c string, inputted as input, and extracts the title from it. numSize is the size of the number section of the cstring and input is the string to be parsed. 
void extractTitle (char *buff, int buffSize, int numSize, char *input, int inSize){
  int lowSize = buffSize;
  char temp[5];

  //myFile = SD.open("/demofile.txt");

  temp[0] = '\0';
  temp[1] = '\0';
  temp[2] = '\0';
  temp[3] = '\0';
  temp[4] = '\0';

  if (inSize < buffSize){
    lowSize = inSize;
  }
  for (int j  = 0; j < buffSize; j++){
    buff[j] = '\0';
  }
  for (int i = (numSize + 4); i < lowSize; i++){
    Serial.print("Math Result: ");
    Serial.println(((i - (numSize+3)) - 4));
    if (((i - (numSize+4)) - 4) >= 0){
      buff[((i - (numSize+4)) - 4)] = temp[0];
    }
    temp[0] = temp[1];
    temp[1] = temp[2];
    temp[2] = temp[3];
    temp[3] = input[i];

    Serial.print("Temp: ");
    Serial.println(temp);
    Serial.print("Buff: ");
    Serial.println(buff);


    Serial.print("String Comp: ");
    Serial.println(strcmp(temp, " -- \0"));
    if (strcmp(temp, " -- \0") == 0){
      Serial.print(buff);
      Serial.println("<");
      //myFile.close();
      return;
    }
    //buff[i - (numSize+3)] = input[i];
  }
  //myFile.close();
  return;
}

void printDetail(uint8_t type, int value){
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!"));
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!"));
      break;
    case DFPlayerUSBInserted:
      Serial.println("USB Inserted!");
      break;
    case DFPlayerUSBRemoved:
      Serial.println("USB Removed!");
      break;
    case DFPlayerPlayFinished:
      Serial.print(F("Number:"));
      Serial.print(value);
      Serial.println(F(" Play Finished!"));
      break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
  
}
