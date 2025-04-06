#include <WiFi.h>
#include <WebServer.h>
#include "secrets.h"  // WiFi credentials

// Designed for the ESP32-Wroom-32 development board
// Create a web server on port 80
WebServer server(80);

// Define which pins are valid analog inputs for your sensors
// Since we're using Wi-Fi, we should limit ourselves to the 6
// available GPIO pins on ADC1
// (Example: GPIO: 32, 33, 34, 35, 36, 39 on ADC1)
/*
We use the GPIO # to address the pin in code
| Pin silkcsreen label| ADC1 #|GPIO # | 
|---------------------|-------|-------|
| VP                  | 0     | 36    |
| VN                  | 3     | 39    |
| D34                 | 6     | 34    |
| D35                 | 7     | 35    |
| D32                 | 4     | 32    |
| D33                 | 5     | 33    |  
*/
int validPins[] = {33, 34, 35, 36, 39};
const int numValidPins = sizeof(validPins) / sizeof(validPins[0]);

// ---------------------------------------------------------------------
// 1) Setup: Connect to Wi-Fi, start the server, define request handler
// ---------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(1000);

  // Connect to your Wi-Fi network
  Serial.print("Connecting to ");
  // Credentials are defined in secrets.h
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PWD);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Define a default handler for all GET requests
  // This will catch paths like /32, /33, etc.
  server.onNotFound(handleRequest);

  // Start the web server
  server.begin();
  Serial.println("HTTP server started.");
}

// ---------------------------------------------------------------------
// 2) Main Loop: Handle incoming client requests
// ---------------------------------------------------------------------
void loop() {
  // Let the server process any incoming requests
  server.handleClient();
}

// ---------------------------------------------------------------------
// 3) Handler: Parse the requested pin from the URL, read ADC, respond
// ---------------------------------------------------------------------
void handleRequest() {
  String path = server.uri();  // e.g. "/32"
  Serial.print("Client requested: ");
  Serial.println(path);

  // Expect the path to be "/<pinNumber>"
  // Remove the leading '/'
  if (path.startsWith("/")) {
    path.remove(0, 1);  // e.g. "32"
  }

  // Convert to integer
  int requestedPin = path.toInt();
  if (requestedPin == 0 && path != "0") {
    // If toInt() returns 0 but the path wasn't "0", it probably wasn't numeric
    server.send(400, "text/plain", "Invalid request: not a valid pin number.");
    return;
  }

  // Check if the requested pin is in our validPins list
  bool isValid = false;
  for (int i = 0; i < numValidPins; i++) {
    if (validPins[i] == requestedPin) {
      isValid = true;
      break;
    }
  }

  if (!isValid) {
    // Pin not in our valid list
    server.send(404, "text/plain", "Error: pin not available or not ADC capable.");
    return;
  }

  // Read from the requested pin
  int reading = analogRead(requestedPin);

  // Build a JSON (or plain text) response
  // Example JSON: {"pin":32,"reading":1234}
  String jsonResponse = "{\"pin\":" + String(requestedPin) + ",\"reading\":" + String(reading) + "}";

  // Send the response
  server.send(200, "application/json", jsonResponse);
}
