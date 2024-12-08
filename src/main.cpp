/*
 * Reads multiple MFRC522 RFID readers on a single SPI bus 
 * and checks that each one matches the predetermined key.
 * The tag-reader pairs are unique so each tag must be placed
 * on the correct reader.
 * 
 * Jordan Carlin
 * jcarlin@hmc.edu
 * 
 * Created: April 22, 2024
 * Modified: April 23, 2024
 */
#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <muddescapes.h>

/*************************************************************************************************/
/** MODIFY THINGS HERE **/
#define NUM_READERS 3 // How many RFID Readers
#define DOOR_PIN 13
#define DEBUG 0

// 26(A0), 25(A1), 4, 21, 13 (led), 27, 33, 15, 32, 14, 23(SDA)

// Reset pins, SDA pins, and correct tags must be listed in the same order
// (each reader must have its info at the same index in the reset pins array,
// the sda pins array, and the correct tags array)
int    rstPins     [NUM_READERS] = {4, 26, 15}; // ESP32 pins connected to reset pins of RFID readers
int    sdaPins     [NUM_READERS] = {21, 25, 32}; // ESP32 pins connected to SDA pins of RFID readers
String correctTags [NUM_READERS] = {"41D2B38", "4598438", "4A8A24F"}; // Correct RFID tag UIDs

/** STOP MODIFICATIONS HERE **/
/*************************************************************************************************/

// Struct for RFID reader that contains the reader itself and the reader number
typedef struct rfidReader{
  MFRC522 mfrc522;
  int     readerNumber;
} rfidReader;

String tagIDs [NUM_READERS]; // array to store current tag IDs
rfidReader readers [NUM_READERS]; // array with each reader object
boolean readerStatus [NUM_READERS]; // if each reader has the correct tag
boolean doorStatus = false; // is door unlocked

// function prototypes
boolean getID(rfidReader reader);
boolean checkIDs();
boolean tagMatch();
void printIDs();
void unlockDoor();
void clearIDs();
void resetPuzzle();

// Control Center connection instantiation
MuddEscapes &me = MuddEscapes::getInstance();
muddescapes_callback callbacks[]{{"Unlock Door", unlockDoor},{"Reset Puzzle", resetPuzzle},{NULL, NULL}};

/*************************************************************************************************/
/** MODIFY THINGS HERE **/
// Add additional readers to the muddescapes_variable for them to appear in the control center. Use the
// same format as the existing variables and make sure not to remove the Null entries at the end.
muddescapes_variable variables[]{{"Reader 0 Status:", &readerStatus[0]},{"Reader 1 Status:", &readerStatus[1]},{"Reader 2 Status:", &readerStatus[2]},{"Door Unlocked:", &doorStatus},{NULL, NULL}};

/** STOP MODIFICATIONS HERE **/
/*************************************************************************************************/
void setup() 
{
  if(DEBUG) {
    Serial.begin(9600);
  }
  SPI.begin(); // SPI bus

  // Initialize each reader object with an rfid reader with appropriate pins and a reader number
  for (int i = 0; i < NUM_READERS; i++) {
    MFRC522 mfrc522_temp = MFRC522(sdaPins[i], rstPins[i]);
    readers[i].mfrc522 = mfrc522_temp;
    readers[i].mfrc522.PCD_Init();
    readers[i].readerNumber = i;
  }

  pinMode(DOOR_PIN, OUTPUT);
  resetPuzzle();

  // Initialize control center connection
  me.init("Claremont-ETC", "Cl@remontI0T", "mqtt://broker.hivemq.com", "Door", callbacks, variables);
}

void loop() 
{
  while (checkIDs()) {
    if(DEBUG) {
      printIDs();
    }
    if (tagMatch()) {
      if(DEBUG) {
        Serial.print("Access Granted!\n");
      }
      unlockDoor();
    } else {
      if(DEBUG) {
        Serial.print("Incorrect tags!\n");
      }
    }
    me.update(); // update control center
    delay(2000);
  }
}

//Read new tag if available
boolean getID(rfidReader reader) 
{
  if ( ! reader.mfrc522.PICC_IsAnyCardPresent()) { //If a tag is placed on RFID reader continue
    tagIDs[reader.readerNumber] = "0000000";
    readerStatus[reader.readerNumber] = false;
    return false;
  }
  if ( ! reader.mfrc522.PICC_ReadCardSerial()) { //Since a tag was placed get Serial and continue
    return false;
  }
  tagIDs[reader.readerNumber] = "";
  for ( uint8_t i = 0; i < 4; i++) { // The MIFARE PICCs that we use have 4 byte UID
    tagIDs[reader.readerNumber].concat(String(reader.mfrc522.uid.uidByte[i], HEX)); // Adds the 4 bytes in a single String variable
  }
  tagIDs[reader.readerNumber].toUpperCase();
  reader.mfrc522.PICC_HaltA(); // Stop reading
  return true;
}

boolean checkIDs(){
  clearIDs();
  boolean result = false;
  for (int i = 0; i < NUM_READERS; i++) {
    result |= getID(readers[i]);
  }
  return result;
}

// Check if all tags match the proper IDs
boolean tagMatch(){
  boolean allMatch = true;
  for (int i = 0; i < NUM_READERS; i++) {
    if (!(tagIDs[i] == correctTags[i])){
      allMatch = false;
      readerStatus[i] = false;
    } else {
      readerStatus[i] = true;
    }
  }
  me.update(); // update control center
  return allMatch;
}

void printIDs(){
  for (int i = 0; i < NUM_READERS; i++){
    Serial.printf("ID %d: %s\n", i, tagIDs[i]);
  }
}

void unlockDoor(){
  digitalWrite(DOOR_PIN, 0);
  doorStatus = true;
  if(DEBUG) {
    Serial.print("Door Unlocked\n");
  }
}

void clearIDs(){
  for (int i = 0; i < NUM_READERS; i++) {
    tagIDs[i] = "0000000";
    readerStatus[i] = false;
  }
}

void resetPuzzle(){
  if(DEBUG) {
    Serial.print("Puzzle Reset\n");
  }
  clearIDs();
  doorStatus = false;
  me.update(); // update control center
  digitalWrite(DOOR_PIN, 1);
}
