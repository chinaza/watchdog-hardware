//Current Sensor power analog input
#define powerPin A0
//GSM Modem definition
#define TINY_GSM_MODEM_SIM800

//GSM LIBRARY
#include <TinyGsmClient.h>
//Software Serial Library
#include <SoftwareSerial.h>

bool thereIsPower();
void alertServer(bool);
bool serverAlertedForPower = false;
bool serverAlertedForNoPower = false;

// Your GPRS credentials
const char apn[]  = "internet.ng.airtel.com";
const char user[] = "internet";
const char pass[] = "internet";

//Web server details
const char server[] = "watchdog.theonlyzhap.xyz";
const char resource[] = "/api/powerstate";
const int port = 80;

//GPRS Serial port
SoftwareSerial gprsSerial(10, 11);

//GSM Initialization
TinyGsm modem(gprsSerial);
TinyGsmClient client(modem);

void setup() {
  // put your setup code here, to run once:

  //Set power pin as input pin
  pinMode(powerPin, INPUT);

  //Initialize serials
  Serial.begin(9600);
  delay(10);
  gprsSerial.begin(9600);
  delay(1000);

  //Prep the GPRS Module
  modem.restart();
}

void loop() {
  // put your main code here, to run repeatedly:
  if (thereIsPower) {
    if (!serverAlertedForPower) alertServer(true);
    serverAlertedForPower = true;
    serverAlertedForNoPower = false;
  } else {
    if (!serverAlertedForNoPower) alertServer(false);
    serverAlertedForNoPower = true;
    serverAlertedForPower = false;
  }
}

bool thereIsPower() {
  int rawValue = analogRead(powerPin);
  double measuredVoltage = (rawValue / 1024.0) * 5000;
  double referenceVoltage = 0.0; //Value should be changed after calibration with ac indicator light

  if (measuredVoltage > referenceVoltage) return true;

  return false;
}

void alertServer(bool powerState) {
  Serial.println("Power state: " + String(powerState));

  Serial.print(F("Waiting for network..."));
  if (!modem.waitForNetwork()) {
    Serial.println(" fail");
    delay(5000);
    return;
  }
  Serial.println(" OK");

  Serial.print(F("Connecting to "));
  Serial.print(apn);
  if (!modem.gprsConnect(apn, user, pass)) {
    Serial.println(" fail");
    delay(5000);
    return;
  }
  Serial.println(" OK");

  Serial.print(F("Connecting to "));
  Serial.print(server);
  if (!client.connect(server, port)) {
    Serial.println(" fail");
    delay(5000);
    return;
  }
  Serial.println(" OK");

  // Make a HTTP POST request:
  client.print(String("POST ") + resource + " HTTP/1.1\r\n");
  client.print(String("Host: ") + server + "\r\n");
  client.print("Content-Type: application/x-www-form-urlencoded\r\n");
  client.print("Cache-Control: no-cache\r\n");
  client.print("dogID=test&state=" + String(powerState) + "\r\n");
  client.print("Connection: close\r\n\r\n");

  unsigned long timeout = millis();
  while (client.connected() && millis() - timeout < 10000L) {
    // Print available data
    while (client.available()) {
      char c = client.read();
      Serial.print(c);
      timeout = millis();
    }
  }
  Serial.println();

  client.stop();
  Serial.println("Server disconnected");

  modem.gprsDisconnect();
  Serial.println("GPRS disconnected");
}

