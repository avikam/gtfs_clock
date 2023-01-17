#define FASTLED_ESP8266_NODEMCU_PIN_ORDER

#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <string>

#include "json.hpp"

#include <FastLED.h>

// How many leds in your strip?
#define NUM_LEDS 8 * 32

// For led chips like WS2812, which have a data line, ground, and power, you just
// need to define DATA_PIN.  For led chipsets that are SPI based (four wires 0 data, clock,
// ground, and power), like the LPD8806 define both DATA_PIN and CLOCK_PIN
// Clock pin only needed for SPI based chipsets when not using hardware SPI
#define DATA_PIN 2

int G_BRIGHTNESS = 2;

// Define the array of leds
CRGB leds[NUM_LEDS];

const char* ssid = "";
const char* password = "";
ESP8266WebServer server(80);

const int led = LED_BUILTIN;

using json = nlohmann::json;

void handleRoot() {
  digitalWrite(led, 1);
  server.send(200, "text/plain", "hello from esp8266!");
  digitalWrite(led, 0);
}

void handleNotFound() {
  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  digitalWrite(led, 0);
}

void clientTest() {
  WiFiClient client;
  HTTPClient http;

  Serial.print("[HTTP] begin...\n");
  const char* headerKeys[] = {
    "x0timestamp"
  };

  http.collectHeaders(headerKeys, 1);
  if (http.begin(client, "http://192.168.1.3:3000/next_train/from/116%20St0Columbia%20University?future_only=true&limit_direction_departures=3")) {  // HTTP

    Serial.print("[HTTP] GET...\n");
    // start connection and send HTTP header
    int httpCode = http.GET();

    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);

      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        // Serial.println(payload);

        String timestamp_str = http.header("x0timestamp");
        int64_t sample_ts = std::stoll(std::string(timestamp_str.c_str()));

        json data = json::parse(payload);
        for (json::iterator it = data.begin(); it != data.end(); ++it) {
          std::string s = (*it)["direction"].get<std::string>();
          int64_t arriving_at = (*it)["arriving_at_timestamp"].get<int64_t>();

          // Serial.println(
          //   String(s.c_str()) + " arrives in " + (arriving_at - sample_ts)/60
          // );
        }
      }
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
  } else {
    Serial.printf("[HTTP} Unable to connect\n");
  }
}

void beginServer() {
  server.on("/", handleRoot);
  server.on("/inline", []() { server.send(200, "text/plain", "this works as well"); });
  server.on("/on", []() {
    digitalWrite(led, 0);
    server.send(200, "text/plain", "this works as well");
  });

  server.on("/off", []() {
    digitalWrite(led, 1);
    server.send(200, "text/plain", "this works as well");
  });

  server.on("/client_test", []() {
    clientTest();
    server.send(200, "text/plain", "this works as well");
  });

  server.onNotFound(handleNotFound);

  server.begin();
}

void setup(void) {
  // pinMode(led, OUTPUT);
  // digitalWrite(led, 0);
  Serial.begin(115200);
  // WiFi.mode(WIFI_STA);
  // WiFi.begin(ssid, password);
  // Serial.println("");

  // // // // Wait for connection
  // while (WiFi.status() != WL_CONNECTED) {
  //   delay(500);
  //   Serial.print(".");
  // }
  // Serial.println("");
  // Serial.print("Connected to ");
  // Serial.println(ssid);
  // Serial.print("IP address: ");
  // Serial.println(WiFi.localIP());

  // if (MDNS.begin("esp8266")) {
  //   Serial.println("MDNS responder started");
  // }
 
  // beginServer();
  // Serial.println("HTTP server started");

  FastLED.addLeds<WS2812B, DATA_PIN, RGB>(leds, NUM_LEDS);  // GRB ordering is typical
  FastLED.setBrightness(G_BRIGHTNESS);
  // FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  Serial.println("Added leds");
}

const int SPACE[][8] = {
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
}; 

const int ONE[][8] = {
  {0, 0, 0, 1, 0, 0, 1, 0,},
  {0, 0, 1, 0, 0, 0, 1, 0,},
  {0, 1, 1, 1, 1, 1, 1, 0,},
  {0, 0, 0, 0, 0, 0, 1, 0,},
};

const int TWO[][8] = {
  {0, 0, 1, 0, 0, 1, 1, 0,},
  {0, 1, 0, 0, 1, 0, 1, 0,},
  {0, 1, 0, 1, 0, 0, 1, 0,},
  {0, 0, 1, 0, 0, 0, 1, 0,},
};

const int THREE[][8] = {
  {0, 0, 1, 0, 0, 1, 0, 0,},
  {0, 1, 0, 0, 0, 0, 1, 0,},
  {0, 1, 0, 1, 1, 0, 1, 0,},
  {0, 0, 1, 0, 0, 1, 0, 0,}
};

const int FOUR[][8] = {
  {0, 1, 1, 1, 0, 0, 0, 0,},
  {0, 0, 0, 1, 0, 0, 0, 0,},
  {0, 0, 0, 1, 0, 0, 0, 0,},
  {0, 1, 1, 1, 1, 1, 1, 0,},
};

const int FIVE[][8] = {
  {0, 1, 1, 1, 0, 0, 1, 0,},
  {0, 1, 0, 1, 0, 0, 1, 0,},
  {0, 1, 0, 1, 0, 0, 1, 0,},
  {0, 1, 0, 0, 1, 1, 0, 0,},
};

const int SIX[][8] = {
  {0, 0, 1, 1, 1, 1, 0, 0,},
  {0, 1, 0, 1, 0, 0, 1, 0,},
  {0, 1, 0, 1, 0, 0, 1, 0,},
  {0, 1, 0, 0, 1, 1, 0, 0,},
};

const int SEVEN[][8] = {
  {0, 1, 0, 0, 1, 0, 0, 0,},
  {0, 1, 0, 0, 1, 0, 0, 0,},
  {0, 1, 1, 1, 1, 1, 1, 0,},
  {0, 0, 0, 0, 1, 0, 0, 0,},
};

const int EIGHT[][8] = {
  {0, 0, 1, 1, 1, 1, 0, 0,},
  {0, 1, 0, 0, 1, 0, 1, 0,},
  {0, 1, 0, 0, 1, 0, 1, 0,},
  {0, 0, 1, 1, 1, 1, 0, 0,},
};

const int NINE[][8] = {
  {0, 0, 1, 1, 0, 0, 1, 0,},
  {0, 1, 0, 0, 1, 0, 1, 0,},
  {0, 1, 0, 0, 1, 0, 1, 0,},
  {0, 0, 1, 1, 1, 1, 0, 0,},
};

const int ZERO[][8] = {
  {0, 0, 1, 1, 1, 1, 0, 0,},
  {0, 1, 0, 0, 0, 0, 1, 0,},
  {0, 1, 0, 0, 0, 0, 1, 0,},
  {0, 0, 1, 1, 1, 1, 0, 0,},
};

int G_OFFSET = 0;

int drawLetter(int ledIx, int w, const int ltr[][8]) {
  if (ledIx - G_OFFSET >=  NUM_LEDS) 
    return ledIx;
  
  for (int i=0; i < w; i++){
    for (int j = 0; j < 8 && ledIx - G_OFFSET < NUM_LEDS; j++, ledIx++) {
      if (ltr[i][ (int(ledIx/8) % 2) ? 8-j -1 : j])
        if (ledIx - G_OFFSET >= 0 && ledIx - G_OFFSET < NUM_LEDS) leds[ledIx - G_OFFSET] = CRGB::Coral;
    }
  }

  return ledIx;
}

void loop(void) { 
  // clientTest();
  for (int i=0; i<NUM_LEDS; i++){
    leds[i] = CRGB::Black;
  }

  int ledIx = 0;

  ledIx = drawLetter(ledIx, 4, ONE);
  ledIx = drawLetter(ledIx, 1, SPACE);

  ledIx = drawLetter(ledIx, 4, TWO);
  ledIx = drawLetter(ledIx, 1, SPACE);

  ledIx = drawLetter(ledIx, 4, THREE);
  ledIx = drawLetter(ledIx, 1, SPACE);

  ledIx = drawLetter(ledIx, 4, FOUR);
  ledIx = drawLetter(ledIx, 1, SPACE);

  ledIx = drawLetter(ledIx, 4, FIVE);
  ledIx = drawLetter(ledIx, 1, SPACE);

  // ledIx = drawLetter(ledIx, 4 - (G_OFFSET), FIVE + G_OFFSET);
  // ledIx = drawLetter(ledIx, 1, SPACE);

  // ledIx = drawLetter(ledIx, 2, FIVE + 2);
  // ledIx = drawLetter(ledIx, 1, SPACE);

  // ledIx = drawLetter(ledIx, 1, FIVE + 3);
  // ledIx = drawLetter(ledIx, 1, SPACE);

  ledIx = drawLetter(ledIx, 4, SIX);
  ledIx = drawLetter(ledIx, 1, SPACE);

  ledIx = drawLetter(ledIx, 4, SEVEN);
  ledIx = drawLetter(ledIx, 1, SPACE);

  ledIx = drawLetter(ledIx, 4, EIGHT);
  ledIx = drawLetter(ledIx, 1, SPACE);

  ledIx = drawLetter(ledIx, 4, NINE);
  ledIx = drawLetter(ledIx, 1, SPACE);

  ledIx = drawLetter(ledIx, 4, ZERO);

  FastLED.show();
  FastLED.delay(500);

  G_OFFSET = (G_OFFSET + 16) % (10*32);

  // FastLED.show();
  // FastLED.delay(1000);
}

/*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
