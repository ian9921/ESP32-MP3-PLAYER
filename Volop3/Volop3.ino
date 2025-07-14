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
int curFold = 1;
int busyLine = 13;
int busyState = 0;
int max_file;

const int NUM_SIZE = 3;
const int FOL_SIZE = 2;
const int NUMS_START = FOL_SIZE + 4;

struct coords {
  int fold;
  int main;
};

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
  curFold = 1;
  myDFPlayer.playFolder(curFold, curSong);  //Play the first mp3
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
  struct coords test;
  int titleTest = 0;
  
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
  
  findNumber(readVals, 200, 237, 4, 20000);
  Serial.print("Find Numer Result: ");
  Serial.println(readVals);
  extractTitle(title, 200, readVals, 200);
  Serial.print("Extract Ttile Result: ");
  Serial.println(title);
  test = extractNums(readVals, 200);
  Serial.print("Folder Coord: ");
  Serial.print(test.fold);
  Serial.print(" Main Coord: ");
  Serial.println(test.main);
  //delay(3000);
  titleTest = checkTitle("Bottom of the Sea", 4, 237);
  Serial.print("Full Match Test (should say 1): ");
  Serial.println(titleTest);
  titleTest = checkTitle("Bottom", 4, 237);
  Serial.print("Partial Match Test (should say 2): ");
  Serial.println(titleTest);
  titleTest = checkTitle("Wellerman", 4, 237);
  Serial.print("No Match Test (should say 0): ");
  Serial.println(titleTest);

  titleTest = checkTitle("The American Dream Is Killing Me", 200, 500);
  Serial.print("Folder and number do not exist (should say -1): ");
  Serial.println(titleTest);

  titleTest = checkTitle("The American Dream Is Killing Me", 2, 500);
  Serial.print("Number does not exist (should say -1): ");
  Serial.println(titleTest);

  titleTest = checkTitle("The American Dream Is Killing Me", 200, 40);
  Serial.print("Folder does not exist (should say -1): ");
  Serial.println(titleTest);

  titleTest = checkTitle("The American Dream Is Killing Me", 2, 40);
  Serial.print("Full Match Test With The (should say 1): ");
  Serial.println(titleTest);
  titleTest = checkTitle("American Dream Is Killing Me", 2, 40);
  Serial.print("Full Match Test Dropping The (should say 1): ");
  Serial.println(titleTest);
  titleTest = checkTitle("American Dream", 2, 40);
  Serial.print("Partial Match Test Dropping The (should say 2): ");
  Serial.println(titleTest); 

  if ((busyState >= 1) && (i >= 40)) {
    delay(500);
    i = 0;
    ready = false;
    check = true;
    Serial.print("Finished Playing ");
    Serial.print(curSong);
    Serial.print(", Now Playing: ");
    curSong++;
    if (curSong > 255){
      curSong = 1;
      curFold++;
    }
    // Serial.println(curSong);
    //delay(100);
    timer = millis();
    myDFPlayer.playFolder(curFold, curSong);  //Play next mp3 every 3 second.
    delay(500);
  }
  
  if (myDFPlayer.available()) {
    printDetail(myDFPlayer.readType(), myDFPlayer.read()); //Print the detail message from DFPlayer to handle different errors and states.
  }
  delay(500);
}

//takes an array and writes to it a list of values to be played next. List size is the size of the array passed. Start variables default to zero but may be raised to skip certain sections
void nameSearch(struct coords *playList, int listSize, int startFold = 0, int startMain = 0, int maxFold = 100, int maxMain = 255){
  struct coords fullMatch[listSize];
  struct coords partMatch[listSize];

  //fill both arrays with impossible values to mark empty spots
  for (int i = 0; i < listSize; i++){
    fullMatch[i].fold = 1000;
    fullMatch[i].main = 1000;
    partMatch[i].fold = 1000;
    partMatch[i].main = 1000;
  }


}

//takes a folder number and main number and returns whether or not the title matches. 0 means no match, 1 means target matches first words, 2 means target appears anywhere in the title, -1 means number does not exist
int checkTitle(char *target, int foldNum, int mainNum){
  char buffer[200];
  char title[200];
  char regTitle[200];
  char regInput[200];
  int c = 0;
  int d = 0;

  int perfect;
  bool imperfect;

  bool inputCap = false;

  struct coords spots;

  bool searchThe = 0;

  buffer[0] = '\0';

  findNumber(buffer, 200, mainNum, foldNum, 20000);
  if (buffer[0] == '\0'){
    return -1;
  }
  extractTitle(title, 200, buffer, 200);
  // Serial.println(target);
  for (int j = 0; j < 200; j++){
    regTitle[j] = '\0';
    regInput[j] = '\0';
  }

  for (int i = 0; i < 200; i++){
    //Serial.println("We Are In The For lOop");
    if (isalnum(title[i])){
      regTitle[c] = title[i];
      //Serial.print(regTitle[d]);
      regTitle[c] = tolower(regTitle[c]);
      c++;
    }
    if (target[i] == '\0'){
      inputCap = true;
    }
    if (inputCap == false)
    {
      if (isalnum(target[i])){
        regInput[d] = target[i];
        //Serial.println(regInput[d]);
        delay(10);
        regInput[d] = tolower(regInput[d]);
        d++;
      } 
    }
  }
  // Serial.print("RegInput: ");
  // Serial.println(regInput);
  regTitle[c] = '\0';
  regInput[d] = '\0';
  c = 0;
  // Serial.print("RegInput: ");
  // Serial.println(regInput);
  
  if ((regInput[0] == 't') && (regInput[1] == 'h') && (regInput[2] == 'e')){
    searchThe = 1;
  } else {
    searchThe = 0;
  }

  if (searchThe == 0){ //eliminates The from the title
    if ((regTitle[0] == 't') && (regTitle[1] == 'h') && (regTitle[2] == 'e')){
      c = 3;
      while(regTitle[c] != '\0'){
        regTitle[c-3] = regTitle[c];
        //Serial.println("We are in the while loop");
        c++;
      }
      regTitle[c-3] = '\0';
    }
  }

  perfect = strcmp(regTitle, regInput);
  // Serial.print("RegTitle: ");
  // Serial.println(regTitle);
  // Serial.print("RegInput: ");
  // Serial.println(regInput);
  if (perfect == 0){
    return 1;
  } else if (strstr(regTitle, regInput) != NULL){
    return 2;
  } else {
    return 0;
  }
}

//takes a given number & folder number and returns the corresponding complete song string in the buffer
void findNumber(char *buff, int size, int target, int folder, int max){
  char tempRead[size];
  char lineNumStr[NUM_SIZE];
  char folNumStr[FOL_SIZE];
  int lineNum;
  int folNum;
  int j = 0;

  myFile = SD.open("/demofile.txt");

  Serial.println("Find Number is Running");

  if (myFile.available()){

    for (int o = 0; o < max; o++){

      for (int p = 0; p < size; p++){
        tempRead[p] = '\0';
      }
      dummy = myFile.read();
      j = 0;

      if (myFile.available() == false){
        Serial.println("End of the Line, time to call it");
        break;
      }

      if (dummy == '\0'){
        Serial.println("End of the Line, time to call it");
        break;
      }

      while ((dummy != '\n') && (dummy != '\0')){
        tempRead[j] = (char)dummy;
        dummy = myFile.read();

        //Serial.println("In Dummy Clear Loop");
        j++;
      }

      Serial.print("tempRead: ");
      Serial.println(tempRead);
      // Serial.println(tempRead);

      for (int q = 0; q < FOL_SIZE; q++){
        folNumStr[q] = tempRead[q];
      }
      for (int m = (NUMS_START); m < (NUMS_START + NUM_SIZE); m++){
        lineNumStr[m-NUMS_START] = tempRead[m];
      }

      lineNum = atoi(lineNumStr);
      folNum = atoi(folNumStr);

      if ((lineNum == target) && (folNum == folder)){
        // Serial.println("Entering Buffer Loop");
        for (int n = 0; n < size; n++){
          buff[n] = tempRead[n];
          // Serial.println("In Buff Write Num Loop");
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
void extractTitle (char *buff, int buffSize, char *input, int inSize){
  int lowSize = buffSize;
  char temp[5];

  //myFile = SD.open("/demofile.txt");

  temp[0] = '\0';
  temp[1] = '\0';
  temp[2] = '\0';
  temp[3] = '\0';
  temp[4] = '\0';

  Serial.println("Extract Title is Running");

  if (inSize < buffSize){
    lowSize = inSize;
  }
  for (int j  = 0; j < buffSize; j++){
    buff[j] = '\0';
  }
  // for (int i = (numSize + 4); i < lowSize; i++){
  for (int i = (NUMS_START + NUM_SIZE + 4); i < lowSize; i++){
    // Serial.print("Math Result: ");
    // Serial.println(((i - (numSize+3)) - 4));
    if (((i - (NUM_SIZE+4)) - 4) >= 0){
      buff[((i - (NUMS_START + NUM_SIZE + 4)) - 4)] = temp[0];
    }
    temp[0] = temp[1];
    temp[1] = temp[2];
    temp[2] = temp[3];
    temp[3] = input[i];

    // Serial.print("Temp: ");
    // Serial.println(temp);
    // Serial.print("Buff: ");
    // Serial.println(buff);


    // Serial.print("String Comp: ");
    // Serial.println(strcmp(temp, " -- \0"));
    if (strcmp(temp, " -- \0") == 0){
      // Serial.print(buff);
      // Serial.println("<");
      //myFile.close();
      return;
    }
    //buff[i - (numSize+3)] = input[i];
  }
  //myFile.close();
  return;
}

//takes c string as input and extracts coordinate numbers.
struct coords extractNums(char *input, int inSize){
  struct coords res;
  char foldNumS[FOL_SIZE+1];
  char mainNumS[NUM_SIZE+1];

  for (int i = 0; i < FOL_SIZE; i++){
    foldNumS[i] = input[i];
  }
  foldNumS[FOL_SIZE] = '\0';
  res.fold = atoi(foldNumS);
  
  for (int m = (NUMS_START); m < (NUMS_START + NUM_SIZE); m++){
    mainNumS[m-NUMS_START] = input[m];
  }
  mainNumS[NUM_SIZE] = '\0';
  res.main = atoi(mainNumS);

  return res;
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
