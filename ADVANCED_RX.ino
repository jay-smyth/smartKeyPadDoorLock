#include <Servo.h> //Including servo library
//Wifi libraries
#include <SPI.h>
#include <WiFi101.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <WiFiSSLClient.h>
#include <WiFiUdp.h>
#include "arduino_secrets.h"


/*
    RX arduino data
    MAC:  F8:F0:05:A6:8D:E5
*/

const byte txPin = 14; //temporary only- This will be changed to Wifi

//Display LED variables
int redLED = 3;
int greenLED = 2;

//Assiging interrupt pins
const int openButton = 0;
const int lockButton = 1;

//servo position variables
int16_t lockPos = 145;
byte x = 0;

//Wifi variable
char ssid[] = SECRET_SSID; //Jason - We'll need to run a hotspot network
char pass[] = SECRET_PASS;//Jason - Our hotspot password
int keyIndex = 0; // network index key (needed if we're using WEP encryption)
int status = WL_IDLE_STATUS;
bool alreadyConnected = false;
WiFiServer server(80);
Servo Servo1; // servo object created from Servo class

/*
    Set wifi connection to the host and displays the current board IP and mac address, host IP and mac address
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
  server.begin();                           // start the web server on port 80
  printWiFiData();                       // you're connected now, so print out the status
  printCurrentNet();
}

void printWiFiData() {  //returns our Tx side IP
  //returns our Tx side IP
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  //  // prints our MAC address:
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

/*
    Interrupt button functions and lock functions, send state to client side
*/

//ISR
void openButtonPressed()
{
  if (digitalRead (openButton) == HIGH) {
    Serial1.write(lockPos); //Send lock position to Client side
    unlock(); //unlock function is called
  }
}

//ISR
void lockButtonPressed()
{
  if (digitalRead (lockButton) == HIGH) {
    Serial1.write(lockPos); //Send lock position to Client side
    lock(); //lock function is called
  }
}

void lock () {
  lockPos = 145;
  Servo1.write(lockPos); //Servo will turn 45 degrees clockwise from position 100 (i.e. lock)..if it has not done so already
  if (Servo1.read() == 145) {
    Serial1.write(lockPos); //Send lock position to Client side
  }
}

void unlock() {
  lockPos = 100;
  Servo1.write(lockPos); //Servo will turn anticlockwise to position 100 (i.e. unlock)..if it has not done so already
  if (Servo1.read() == 100) {
    Serial1.write(lockPos); //Send lock position to Client side
  }
}



void setup() {
  pinMode (txPin, INPUT); //temporary only- This will be changed to Wifi
  Serial.begin(9600);
  Serial1.begin(9600); //Keeping serial comms to provide lock state
  pinMode (redLED, OUTPUT); // Setting the red LED as output
  pinMode (greenLED, OUTPUT); //Setting the green LED as output
  Servo1.attach(6); //Configuring pin 6 as servo control
  Servo1.write(lockPos); //lock intially set to position 145 (i.e. locked)
  digitalWrite (openButton, HIGH);
  digitalWrite (lockButton, HIGH);
  attachInterrupt (0, openButtonPressed, CHANGE);//When change in voltage ISR is called
  attachInterrupt (1, lockButtonPressed, CHANGE);//When change in voltage ISR is called
  setWifi();
}


void loop() {
  Serial1.write(lockPos);//set initial lock position(Locked)
  WiFiClient client = server.available();   // listen for incoming clients
/*
    Set wifi connection to the host, recieve and hadle HTTP request and process sent commands.
    Allow central hub to control Server Servo and change lock position
*/
  if (client) {                             // if you get a client,
    Serial.println("new client");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK"); //had to newline this to get a command sent back

            client.println("Content-type:text/html");
            client.println();

            /*
                        The below will provide a html document to allow interaction with the door through a browser when
                        the server IP is entered.
            */
            // the content of the HTTP response follows the header:
            client.print("Click <a href=\"/Serv-unlock\">here</a> Unlock the door<br>");
            client.print("Click <a href=\"/Serv-lock\">here</a> Lock the door<br>");


            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          }
          else {      // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        }
        else if (c != '\r') {    // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // Check to see what the client request was:

        if (currentLine.endsWith("GET /Serv-unlock")) {
          unlock();            // GET /Serv-unlock calls the unlock function
        }
        if (currentLine.endsWith("GET /Serv-lock")) {
          lock();                // GET /L-btn turns the built in LED off
        }

      }
    }
    // close the connection:
    client.stop();
    Serial.println("client disonnected");
  }
  //test lock position and display 
  if (lockPos == 100) {
    digitalWrite (greenLED, HIGH);
    digitalWrite (redLED, LOW);
  } else if (lockPos == 145) {
    digitalWrite (redLED, HIGH);
    digitalWrite (greenLED, LOW);
  }
}
