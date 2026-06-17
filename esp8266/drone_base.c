#include <SoftwareSerial.h>

#define SIM7600_RX D2
#define SIM7600_TX D1
#define SIM7600_PWRKEY D3

const char* SERVER = "your-server.com";
const char* POLL_ENDPOINT = "/poll";

double latitude = 0.0;
double longitude = 0.0;
double altitude = 0.0;

SoftwareSerial sim7600(SIM7600_RX, SIM7600_TX);

void setup() {
  Serial.begin(115200);
  sim7600.begin(115200);
  pinMode(SIM7600_PWRKEY, OUTPUT);
  
  powerOnSIM7600();
  delay(10000);
  initSIM7600();
  checkNetworkRegistration();
}

void loop() {
  String response = sendHTTPRequest();
  
  if (response.length() > 0) {
    parseCoordinates(response);
    Serial.print("Latitude: "); Serial.println(latitude, 6);
    Serial.print("Longitude: "); Serial.println(longitude, 6);
    Serial.print("Altitude: "); Serial.println(altitude, 2);
  }
  
  delay(30000);
}

void powerOnSIM7600() {
  digitalWrite(SIM7600_PWRKEY, HIGH);
  delay(1000);
  digitalWrite(SIM7600_PWRKEY, LOW);
  delay(3000);
}

void initSIM7600() {
  sendATCommand("AT+IPR=115200", 1000);
  sendATCommand("ATE1", 1000);
  sendATCommand("AT+CGDCONT=1,\"IP\",\"your.apn.com\"", 1000);
  sendATCommand("AT+CNMP=2", 1000);
  sendATCommand("AT+CGATT=1", 3000);
}

void checkNetworkRegistration() {
  String response = sendATCommand("AT+CREG?", 1000);
  if (response.indexOf("+CREG: 0,1") > 0 || response.indexOf("+CREG: 0,5") > 0) {
    Serial.println("Network registered");
  } else {
    Serial.println("Network not registered");
  }
}

String sendHTTPRequest() {
  String httpRequest = "GET " + String(POLL_ENDPOINT) + " HTTP/1.1\r\n";
  httpRequest += "Host: " + String(SERVER) + "\r\n";
  httpRequest += "Connection: close\r\n";
  httpRequest += "\r\n";
  
  sendATCommand("AT+HTTPINIT", 1000);
  sendATCommand("AT+HTTPPARA=\"CID\",1", 1000);
  sendATCommand("AT+HTTPPARA=\"URL\",\"http://" + String(SERVER) + String(POLL_ENDPOINT) + "\"", 1000);
  
  String response = sendATCommand("AT+HTTPACTION=0", 5000);
  String result = sendATCommand("AT+HTTPREAD", 5000);
  
  sendATCommand("AT+HTTPTERM", 1000);
  
  return result;
}

String sendATCommand(String command, int timeout) {
  String response = "";
  
  sim7600.println(command);
  delay(100);
  
  long startTime = millis();
  while (millis() - startTime < timeout) {
    while (sim7600.available()) {
      char c = sim7600.read();
      response += c;
    }
    if (response.indexOf("OK") > 0 || response.indexOf("ERROR") > 0) {
      break;
    }
  }
  
  Serial.println("AT Command: " + command);
  Serial.println("Response: " + response);
  return response;
}

void parseCoordinates(String response) {
  int latIndex = response.indexOf("\"lat\"");
  if (latIndex > 0) {
    int colonIndex = response.indexOf(":", latIndex);
    int commaIndex = response.indexOf(",", colonIndex);
    String latStr = response.substring(colonIndex + 1, commaIndex);
    latitude = latStr.toDouble();
  }
  
  int lngIndex = response.indexOf("\"lng\"");
  if (lngIndex > 0) {
    int colonIndex = response.indexOf(":", lngIndex);
    int commaIndex = response.indexOf(",", colonIndex);
    String lngStr = response.substring(colonIndex + 1, commaIndex);
    longitude = lngStr.toDouble();
  }
  
  int altIndex = response.indexOf("\"alt\"");
  if (altIndex > 0) {
    int colonIndex = response.indexOf(":", altIndex);
    int closeBraceIndex = response.indexOf("}", colonIndex);
    String altStr = response.substring(colonIndex + 1, closeBraceIndex);
    altitude = altStr.toDouble();
  }
}

void getCurrentCoordinates(double* lat, double* lng, double* alt) {
  *lat = latitude;
  *lng = longitude;
  *alt = altitude;
}
