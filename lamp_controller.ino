// Webserver stuff

void setupServerRoutes();
void setupFilePaths();

#define TEMPLATE_PLACEHOLDER '@'

#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

// Neopixel Stuff
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

#define PIN D6
#define NUMPIXELS 30

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
#define DELAYVAL 500
#define BUTTON D2

// Filesystem Stuff
#include "LittleFS.h"


// State
bool lastButtonWasLOW = false;
bool buttonOn = false;
uint32_t currentColor = pixels.Color(233, 161, 0);
String currentColorHex = "e9a100";
// yellow 233, 161, 0
// red 233, 136, 30

// Webserver Stuff
const char *ssid = "<REPLACE_ME>";
const char *password = "<REPLACE_ME>";

//Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket webSocket("/ws");  // WebSocket on port 81


void notifyClients(String message) {
  webSocket.textAll(message);
}

void logMessage(String msg) {
  Serial.println(msg);
  notifyClients(msg);
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void notFound(AsyncWebServerRequest *request) {
  AsyncWebServerResponse *response =
    request->beginResponse(LittleFS, "/404.html", "text/html");
  response->setCode(404);
  request->send(response);
}


void setup() {
  delay(500);
  Serial.println();
  delay(500);

  pixels.begin();


  // Initialize the serial port
  Serial.begin(115200);

  // Configure BUTTON pin as an input with a pullup
  pinMode(BUTTON, INPUT_PULLUP);

  // Init Filesystem
  if (!LittleFS.begin()) {
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }

  // Wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  unsigned long startAttemptTime = millis();

  // Try for up to 30 seconds (30000 ms)
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 30000) {
    delay(500);
    Serial.print(".");
  }

  Serial.print("Verbunden! IP-Adresse: ");
  Serial.println(WiFi.localIP());


  setupServerRoutes();
  setupFilePaths();

  server.begin();

  webSocket.onEvent(onEvent);
  server.addHandler(&webSocket);

  logMessage("Startup Finished");
}

void loop() {

  int buttonState = digitalRead(BUTTON);

  if (buttonState == LOW && !lastButtonWasLOW) {
    buttonOn = true;
    lastButtonWasLOW = true;
    turn_pixels_on();
  } else if (buttonState != LOW && lastButtonWasLOW) {
    buttonOn = false;
    lastButtonWasLOW = false;
    turn_pixels_off();
  }

  delay(500);
}

void turn_pixels_on() {
  logMessage("Button set to on!");
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, currentColor);
    pixels.show();
  }
}

void turn_pixels_off() {
  pixels.clear();
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 0, 0));
    pixels.show();
  }
  logMessage("Button set to off!");
}


String processor(const String &var) {

  Serial.println(var);

  if (var == "PLACEHOLDER_CURRENT_COLOR") {
    return currentColorHex;
  } else if (var == "PLACEHOLDER_STATUS") {
    if (buttonOn) {
      return "on";
    } else {
      return "off";
    }
  }

  return String();
}

void setupServerRoutes() {
  // Datei `index.html` wird ausgeliefert
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html", false);
  });

  // Datei `index.html` wird ausgeliefert
  server.on("/logs", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/log.html");
  });


  server.on("/color", HTTP_POST, [](AsyncWebServerRequest *request) {
    logMessage("Received POST /color");

    const AsyncWebParameter *p = request->getParam(0);
    const String color = p->value().c_str();

    // RRGGBB = 6 chars + null terminator
    char hexChars[8];  // 6 hex digits + null terminator
    color.substring(1).toCharArray(hexChars, sizeof(hexChars));

    long number = strtol(hexChars, NULL, 16);
    int r = (number >> 16) & 0xFF;
    int g = (number >> 8) & 0xFF;
    int b = number & 0xFF;


    logMessage("Color in rgb: (" + String(r) + ", " + String(g) + ", " + String(b) + ")");
    currentColor = pixels.Color(r, g, b);
    currentColorHex = color.substring(1);

    if (buttonOn) {
      turn_pixels_on();
    }

    request->redirect("/");
  });

  server.on("/turnon", HTTP_POST, [](AsyncWebServerRequest *request) {
    buttonOn = true;
    turn_pixels_on();

    request->redirect("/");
  });

  server.on("/turnoff", HTTP_POST, [](AsyncWebServerRequest *request) {
    buttonOn = false;
    turn_pixels_off();

    request->redirect("/");
  });

  server.on("/color", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Send 200 OK response
    request->send(200, "text/plain", currentColorHex);
  });

  server.onNotFound(notFound);
}

void setupFilePaths() {
  server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/styles.css", "text/css");
  });

  server.on("/mothman.png", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/mothman.png", "image/png");
  });
}
