#include <LiquidCrystal.h> // Including LCD library
#include <Password.h> //Including Password library

//Wifi libraries
#include <SPI.h>
#include <WiFi101.h>
#include "arduino_secrets.h"


/*
    TX arduino data
    MAC:  F8:F0:05:EC:99:C0
*/

//Wifi variables
char ssid[] = SECRET_SSID; //Jason - We'll need to run a hotspot network
char pass[] = SECRET_PASS;//Jason - Our hotspot password
int keyIndex = 0; // network index key (needed if we're using WEP encryption)
int status = WL_IDLE_STATUS;
bool alreadyConnected = false;
WiFiClient client;

//##### Attention ###### this variable will need updated with the Server(RX) sides IP
IPAddress ISY(192, 168, 137, 51);

//Serial communication variable - Will stay to return state from RX
const byte rxPin = 13;


//LCD Variables
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

//Keypad variables
const int rowArrayPins[] = {A5, A6, 0, 1};
const int colArrayPins[] = {A4, A3, A2, A1};
const int rows = 4;
const int cols = 4;
const char keyPress[rows][cols] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

//Display LED variables
int redLED = 8;
int greenLED = 7;

//password variables
Password password = Password("2019");
int maxPasswordLength = 4;
int currentPasswordLength = 0;

//buzzer variable
const int buzzer = 10;

//servo position variables
int16_t lockPos = 0;

/*
    Set wifi connection to the host and displays the current board IP and mac address, host IP and mac address,
    listens for response from the MKR server(Receiver)
*/

void setWifi () {
  // Check for the presence of the shield, can probably drop this not sure cause I can't run it
  Serial.print("WiFi101 shield: ");
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("NOT PRESENT");
    return; // don't continue
  }
  Serial.println("DETECTED");
  // attempt to connect to Wifi network: once connected if we can find a second device we can send stuff
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to Network named: ");
    Serial.println(ssid);                   // print the network name (SSID);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    // wait 10 seconds for connection:
    delay(10000);
  }
  printWiFiData();
  printCurrentNet();
}

void printWiFiData() {
  //returns our Tx side IP
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // prints our MAC address:
  byte mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC address: ");
  printMacAddress(mac);
}

void printCurrentNet() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print the MAC address of the router you're attached to:
  byte bssid[6];
  WiFi.BSSID(bssid);
  Serial.print("BSSID: ");
  printMacAddress(bssid);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.println(rssi);

  // print the encryption type:
  byte encryption = WiFi.encryptionType();
  Serial.print("Encryption Type:");
  Serial.println(encryption, HEX);
  Serial.println();
}

void printMacAddress(byte mac[]) {
  for (int i = 5; i >= 0; i--) {
    if (mac[i] < 16) {
      Serial.print("0");
    }
    Serial.print(mac[i], HEX);
    if (i > 0) {
      Serial.print(":");
    }
  }
  Serial.println();
}

// listen for and print response from MKR server
void listenToClient() {
  String currentLine = "";
  unsigned long startTime = millis();
  bool received = false;

  while ((millis() - startTime < 5000) && !received) { // listen for 5 seconds
    while (client.available()) {
      received = true;
      char c = client.read();
      Serial.write(c);
    }
  }
  client.stop();
  Serial.println();
}

/*
   Lock and unlock functions below, the lock and unlock functions display the state of input and displays to the user,
    the lockSend and unlockSend functions transmit to the server(receiver)
*/

void lock() {
  clearScreen();
  lcd.setCursor(0, 0);
  lcd.print("LOCKED");
  wrongRedFlash ();//Jason - just made this a function so it wasn't in the way
  errorBuzz();
  lockSend();
  delay(1000);
  enterPasswordScreen();
}

void lockSend () {
  if (client.connect(ISY, 80)) { //check if connection to MKR server is made
    Serial.println("Connected");

    //Make a HTTP request to set state variable 1
    client.println(" HTTP/1.1");
    client.println("Host: 192.168.0.16");
    client.println("GET /Serv-lock");
    client.println("Content-Type: text/html");
    client.print("Sending lock command");
    client.println();

    //Print variable change of reply from server
    Serial.print("Variable Changed to ");
    listenToClient();   //Listen for MKR server response to previous request
  }
  else {
    Serial.println("Connection Failed");
  }
}

void unlock() {
  clearScreen();
  lcd.setCursor(0, 0);
  lcd.print("UNLOCKED");
  correctBuzz();
  correctGrnFlash();
  Serial.print(lockPos);
  unlockSend();
  enterPasswordScreen();
}

//function to send unlock variable via http to RX
void unlockSend () {
  if (client.connect(ISY, 80)) { //check if connection to MKR server is made
    Serial.println("Connected");

    //Make a HTTP request to set state variable 1
    client.println(" HTTP/1.1");
    client.println("GET /Serv-unlock");
    client.println("Content-Type: text/html");
    client.print("Sending lock command");
    client.println();

    //Print variable change of reply from server
    Serial.print("Variable Changed to ");
    listenToClient();   //Listen for MKR server response to previous request
  }
  else {
    Serial.println("Connection Failed");
  }
}

/*
   Keypad validation and LCD display functions

*/

void enterPasswordScreen() {
  lcd.setCursor(0, 0);
  lcd.print("Enter Password");
}

void clearScreen() {
  lcd.setCursor(0, 0);
  lcd.print("                ");
}

void invalidPassword() {
  lcd.setCursor(0, 1);
  lcd.print("Invalid");
  delay(1000);
  wrongRedFlash();//Jason - just made this a function so it wasn't in the way
  errorBuzz();
  enterPasswordScreen();
}

void PasswordReset() {
  password.reset();
  currentPasswordLength = 0;
  lcd.setCursor(0, 1);
  lcd.print("                ");
}

char ProcessPassword(char keyPress) {
  password.append(keyPress); //adding value of keyPress variable to guessed password
  enterPasswordScreen();
  lcd.setCursor(0, 1);
  lcd.print(keyPress);
}

void PasswordCheck() {
  if (password.evaluate()) {
    unlock();
  } else {
    clearScreen();
    invalidPassword();
  }
  PasswordReset();
}

/*
   Keypad entry
*/

char readKey() { //will probably do away with this and go for keypad library instead
  for (int row = 0; row < 4; row++) {
    digitalWrite(rowArrayPins[0], HIGH);
    digitalWrite(rowArrayPins[1], HIGH);
    digitalWrite(rowArrayPins[2], HIGH);
    digitalWrite(rowArrayPins[3], HIGH);
    digitalWrite(rowArrayPins[row], LOW);
    for (int col = 0; col < 4; col++) {
      int colState = digitalRead(colArrayPins[col]);
      if (colState == LOW) {
        return keyPress[row][col];
      }
    }
  }
  return '\n';
}

/*
   Display feedback through periphal modules, buzzers and LEDs
*/

void correctGrnFlash() {
  for (int i = 0; i < 4; i++) {
    digitalWrite (greenLED, HIGH);
    delay(100);
    digitalWrite (greenLED, LOW);
    delay(100);
  }
}

void wrongRedFlash () {
  for (int i = 0; i < 4; i++) {
    digitalWrite (redLED, HIGH);
    delay(50);
    digitalWrite (redLED, LOW);
    delay(50);
  }
}

void errorBuzz() {
  tone(buzzer, 100); // very low tone to indicate deadlock
  delay(800);
  noTone(buzzer);//buzzer will turn off
}

void correctBuzz() {
  tone(buzzer, 100); // very low tone to indicate deadlock
  delay(800);
  noTone(buzzer);//buzzer will turn off
}



void setup() {
  pinMode (rxPin, OUTPUT);
  Serial.begin(9600);
  Serial1.begin(9600); //temporary only- This will be changed to Wifi
  lcd.begin(16, 2);
  enterPasswordScreen(); //this function will be called initially on start up
  setWifi();

  lcd.cursor();

  for (int i = 0; i < 4; i++) {
    pinMode(rowArrayPins[i], OUTPUT);
    digitalWrite (rowArrayPins[i], HIGH);

    if (i < 4) {
      pinMode(colArrayPins[i], INPUT_PULLUP);
    }
  }
  pinMode (redLED, OUTPUT);
  pinMode (greenLED, OUTPUT);
  pinMode (buzzer, OUTPUT); //configuring buzzer as output
}


void loop() {
  char keyPress = readKey();
  if (keyPress != '\n') {
    Serial.print(keyPress);
    delay(500);
    switch (keyPress) {
      case '*': PasswordReset();//function is called if the 'star' key is pressed
        break;
      case '#': PasswordCheck();//function is called if the 'hash' key is pressed
        break;
      case 'D': lock();//function is called if the 'D' key is pressed
        break;
      default: ProcessPassword(keyPress);//function is called if any other key is pressed
        break;
    }
  }
  lockPos = Serial1.read() - 0;
  if (lockPos == 100) {
    digitalWrite (greenLED, HIGH);
    digitalWrite (redLED, LOW);
  } else if (lockPos == 145) {
    digitalWrite (redLED, HIGH);
    digitalWrite (greenLED, LOW);
  }
}
