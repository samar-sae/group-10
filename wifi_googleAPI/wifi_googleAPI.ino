#include <Smartcar.h>
#include <Wire.h>
#include <VL53L0X.h>
#include <ESPmDNS.h>



#include <Arduino.h>
#include <Arduino_JSON.h>
#ifdef ARDUINO_ARCH_SAMD
#include <WiFi101.h>
#elif defined ARDUINO_ARCH_ESP8266
#include <ESP8266WiFi.h>
#elif defined ARDUINO_ARCH_ESP32
#include <WiFi.h>
#else
#error Wrong platform
#endif 

#include <WifiLocation.h>

char input;
int forwardSpeed = 40;
int backSpeed = -40;
int brake = 0;
int millimeterLimit = 200;
int lDegrees = -10; // degrees to turn left
int rDegrees = 10;  // degrees to turn right
int accelerate = 10;
int decelerate = -10;
int diffSpeed = 5;
int currentSpeed;
const unsigned long PRINT_INTERVAL = 100;
unsigned long previousPrintout = 0;
const auto pulsesPerMeter = 600;

const char* ssid     =  "ssid";
const char* password = "password";
const char* googleApiKey = "APIkey";

WifiLocation location(googleApiKey);
WiFiServer server(80);

BrushedMotor leftMotor(smartcarlib::pins::v2::leftMotorPins);
BrushedMotor rightMotor(smartcarlib::pins::v2::rightMotorPins);
DifferentialControl control(leftMotor, rightMotor);
GY50 gyroscope(37);

DirectionlessOdometer leftOdometer(
    smartcarlib::pins::v2::leftOdometerPin,
    []() { leftOdometer.update(); },
    pulsesPerMeter);
DirectionlessOdometer rightOdometer(
    smartcarlib::pins::v2::rightOdometerPin,
    []() { rightOdometer.update(); },
    pulsesPerMeter);

SmartCar car(control, gyroscope, leftOdometer, rightOdometer);
VL53L0X sensor;

void setup() {
  Serial.begin(9600);
  Wire.begin();
  sensor.setTimeout(500);

#ifdef ARDUINO_ARCH_ESP32
    WiFi.mode(WIFI_MODE_STA);
#endif
#ifdef ARDUINO_ARCH_ESP8266
    WiFi.mode(WIFI_STA);
#endif
    WiFi.begin(ssid, password);
  
 Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print("Attempting to connect to WPA SSID: ");
        Serial.println(ssid);
         Serial.print("Status = ");
        Serial.println(WiFi.status());
         delay(500);
    }

    location_t loc2 = location.getGeoFromWiFi();

    Serial.println("Location request data");
    Serial.println(location.getSurroundingWiFiJson());
    Serial.println("Latitude: " + String(loc2.lat, 7));
    Serial.println("Longitude: " + String(loc2.lon, 7));
    Serial.println("Accuracy: " + String(loc2.accuracy));


    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    if (MDNS.begin("magess")) {

      Serial.println("magess.local is up");
    }
    
    server.begin();
   
  if (!sensor.init())
  {
    Serial.println("Failed to detect and initialize sensor!");
    while (1) {}
  }

  // Start continuous back-to-back mode (take readings as
  // fast as possible).  To use continuous timed mode
  // instead, provide a desired inter-measurement period in
  // ms (e.g. sensor.startContinuous(100)).
  sensor.startContinuous();
}

int value = 0;

void loop() {
   
   Serial.println(sensor.readRangeContinuousMillimeters());
    Serial.println();
    WiFiClient client = server.available();   // listen for incoming clients
    if (client) {                             // if you get a client,
    Serial.println("New Client.");           // print a message out the serial port
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
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // the content of the HTTP response follows the header:
            client.print("Click <a href=\"/F\">here</a> to turn on the car.<br>");
            client.print("Click <a href=\"/S\">here</a> to turn off the car.<br>");
            client.print("Click <a href=\"/L\">here</a> to turn left with the car.<br>");
            client.print("Click <a href=\"/R\">here</a> to turn right with the car.<br>");
            client.print("Click <a href=\"/B\">here</a> to go backwards with the car.<br>");
            client.print("Click <a href=\"/M\">here</a> to get location");
            
            

            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
        // Check to see if the client request was "GET /F" or "GET /S":
        if (currentLine.endsWith("GET /F")) {
          car.setSpeed(forwardSpeed);         // GET /F makes the car run forward
        }
        if (currentLine.endsWith("GET /S")) {
          car.setSpeed(brake);                // GET /S makes the car stop
        }
        if (currentLine.endsWith("GET /B")) {
          car.setSpeed(backSpeed);            // GET /B makes the car go backward
        }
        if (currentLine.endsWith("GET /L")){
          car.setAngle(lDegrees); 
          car.setSpeed(forwardSpeed); 
                                              // GET /L makes the car go to the left
        }
        if (currentLine.endsWith("GET /R")) {
          car.setAngle(rDegrees);
          car.setSpeed(forwardSpeed);             // GET /R makes the car go to the right
        }
        if (currentLine.endsWith("GET /M")){
        location_t loc = location.getGeoFromWiFi();
          client.print("Lat: " + String(loc.lat, 7));
          client.print("Lon: " + String(loc.lon, 7));
         
        }
        
         if(sensor.readRangeContinuousMillimeters()< 250){
        car.setSpeed(0);
        delay(500);
        car.setSpeed(-10);
        delay(3000);
        car.setSpeed(0);
        }
      }
    }
     
      }
    // close the connection:
    client.stop();
    }