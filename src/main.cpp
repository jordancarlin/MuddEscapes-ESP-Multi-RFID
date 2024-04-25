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
#define NUM_READERS 4 // How many RFID Readers

// 26(A0), 25(A1), 4, 21, 13 (led), 27, 33, 15, 32, 14, 23(SDA)

// int rstPins [NUM_READERS] = {4, 32, 33}; // ESP32 pins connected to reset pins of RFID readers
// int sdaPins [NUM_READERS] = {23, 14, 21}; // ESP32 pins connected to SDA pins of RFID readers

// Reset pins, SDA pins, and correct tags must be listed in the same order
// (each reader must have its info at the same index in the reset pins array,
// the sda pins array, and the correct tags array)
int rstPins [NUM_READERS] = {14, 4, 26, 15}; // ESP32 pins connected to reset pins of RFID readers
int sdaPins [NUM_READERS] = {23, 21, 25, 32}; // ESP32 pins connected to SDA pins of RFID readers
String correctTags [NUM_READERS] = {"4642539", "41D2B38", "4598438", "4A8A24F"}; // Correct RFID tag UIDs

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
muddescapes_variable variables[]{{"Reader 0 Status:", &readerStatus[0]},{"Reader 1 Status:", &readerStatus[1]},{"Reader 2 Status:", &readerStatus[2]},{"Reader 3 Status:", &readerStatus[3]},{"Door Unlocked:", &doorStatus},{NULL, NULL}};

void setup() 
{
  Serial.begin(9600);
  SPI.begin(); // SPI bus

  // Initialize each reader object with an rfid reader with appropriate pins and a reader number
  for (int i = 0; i < NUM_READERS; i++) {
    MFRC522 mfrc522_temp = MFRC522(sdaPins[i], rstPins[i]);
    readers[i].mfrc522 = mfrc522_temp;
    readers[i].mfrc522.PCD_Init();
    readers[i].readerNumber = i;
  }
  resetPuzzle();
  // Initialize control center connection
  me.init("Claremont-ETC", "Cl@remontI0T", "mqtt://broker.hivemq.com", "Door", callbacks, variables);
}

void loop() 
{
  //Wait until new tag is available
  while (checkIDs()) {
    printIDs(); // Testing only
    if (tagMatch()) {
      Serial.print("Access Granted!\n"); // Testing only
      unlockDoor();
    } else {
      Serial.print("Incorrect tags!\n"); // Testing only
    }
    me.update(); // update control center
    delay(2000);
  }
}

//Read new tag if available
boolean getID(rfidReader reader) 
{
  if ( ! reader.mfrc522.PICC_IsNewCardPresent()) { //If a new tag placed on RFID reader continue
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
  return allMatch;
}

void printIDs(){
  for (int i = 0; i < NUM_READERS; i++){
    Serial.printf("ID %d: %s\n", i, tagIDs[i]);
  }
}

void unlockDoor(){
  // ADD CODE TO UNLOCK DOOR
  doorStatus = true;
  Serial.print("Door Unlocked\n");
}

void clearIDs(){
  for (int i = 0; i < NUM_READERS; i++) {
    tagIDs[i] = "0000000";
    readerStatus[i] = false;
  }
}

void resetPuzzle(){
  Serial.print("Puzzle Reset\n");
  clearIDs();
  doorStatus = false;
  // ADD CODE TO LOCK DOOR
}
