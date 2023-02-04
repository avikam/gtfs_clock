#define FASTLED_ESP8266_NODEMCU_PIN_ORDER

#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <FastLED.h>
#include <LittleFS.h>

#include <string>
#include <vector>

#include <NextStopClient.hpp>
#include <Config.hpp>
#include <symbol.hpp>

// How many leds in your strip?
#define LED_COLS 32
#define NUM_LEDS (8*LED_COLS)

// For led chips like WS2812, which have a data line, ground, and power, you just
// need to define DATA_PIN.  For led chipsets that are SPI based (four wires 0 data, clock,
// ground, and power), like the LPD8806 define both DATA_PIN and CLOCK_PIN
// Clock pin only needed for SPI based chipsets when not using hardware SPI
#define DATA_PIN 2

int G_BRIGHTNESS = 2;

// Define the array of leds
CRGB leds[NUM_LEDS];

static const char* deviceName = "NextStop"; 
static const char* timestampHeader = "x-timestamp";
static const char* headerKeys[] = {
    timestampHeader
};
#define HEADERS_KEYS_SIZE 1

#define IDLE_CYCLES 10
using vec_iter = std::vector<int>::iterator;

class NextStopClient {
  std::string _url;

  public:
  NextStopClient() {};

  void setUrl(const std::string&& url) {
    _url = url;
  }

  int getNextStops(vec_iter begin, const vec_iter& end) {
    if (_url.length() == 0) {
      Serial.print("Client not configured!\n");
      return 0;
    }

    if (begin == end) {
      Serial.print("Err: empty result buffer\n");
      return 0;
    }

    WiFiClient client;
    HTTPClient http;

    Serial.print("[HTTP] begin...\n");

    http.collectHeaders(headerKeys, HEADERS_KEYS_SIZE);
    if (!http.begin(client, String(_url.c_str()))) {  // HTTP
      Serial.printf("[HTTP} Unable to connect\n");
      return 0;
    }

    Serial.print("[HTTP] GET...\n");
    // start connection and send HTTP header
    int httpCode = http.GET();

    // httpCode will be negative on error
    if (httpCode <= 0) {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
        http.end();
        return 0;
    }

    // HTTP header has been send and Server response header has been handled
    Serial.printf("[HTTP] GET... code: %d\n", httpCode);

    if (httpCode != HTTP_CODE_OK) {
      http.end();
      return 0;
    }

    auto payload = http.getString();
    auto timestamp_str = http.header(timestampHeader);

    auto result = next_stop::next_minutes(timestamp_str.c_str(), payload);
    int c = 0;
    for (auto i : result) {
      *begin = i; c++;
      if (++begin == end) { break; }
    }

    http.end();
    return c;
  }
};

void setupWifi(const char* ssid, const char* password) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin(deviceName)) {
    Serial.println("MDNS responder started");
  }  
}

void setupLeds() {
  FastLED.addLeds<WS2812B, DATA_PIN, RGB>(leds, NUM_LEDS);  // GRB ordering is typical
  FastLED.setBrightness(G_BRIGHTNESS);
  // Serial.println("Added leds");
}

void drawColumn(int col, const char* row, CRGB color){
  if (col < 0 || col >= LED_COLS) {
    Serial.printf("Error col bounds (%d)\n", col);
    return;
  }

  for (int i = 0 ; i < 8; i++) {
    auto d = row[(col % 2) == 0 ? i : 7 - i];
    auto pix = col * 8 + i;
    if (pix < NUM_LEDS) leds[pix] = color * (uint8_t)d;
  }
}

template <class Sym, class Iter>
int drawSymbol(int col, const symbol::ISymbol<Sym, Iter>& symbol, CRGB color, int offset=0, int start=0, int end=LED_COLS) {
  end = min(end, LED_COLS);
  start = max(start, 0);

  if (col - offset >=  end) 
    return col;
  
  // if (col - offset + symbol.cols() < start) {
  //   return col + symbol.cols();
  // }

  for (auto row : symbol.rows_iter()) {
    auto c = col - offset;
    if (c >= start && c < end) drawColumn(c, row, color);
    col++;
  }
  return col;
}

void listDir(const char * dirname) {
  Serial.printf("Listing directory: %s\n", dirname);

  Dir root = LittleFS.openDir(dirname);

  while (root.next()) {
    File file = root.openFile("r");
    Serial.print("  FILE: ");
    Serial.print(root.fileName());
    Serial.print("  SIZE: ");
    Serial.print(file.size());
    time_t cr = file.getCreationTime();
    time_t lw = file.getLastWrite();
    file.close();
    struct tm * tmstruct = localtime(&cr);
    Serial.printf("    CREATION: %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
    tmstruct = localtime(&lw);
    Serial.printf("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
  }
}

 bool readConfig(const char * path, config::Config* conf) {
  Serial.printf("Reading file: %s\n", path);

  File file = LittleFS.open(path, "r");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return false;
  }

  StreamString configStream;
  while (file.available()) {
    file.sendAll(configStream);
  }
  file.close();

  return config::parse(configStream, conf);
}


struct LoopState {
  std::vector<int> currentNextStops;
  int region_offset;
  symbol::SymbolArray syms;
  int cyclesSinceRequest;
  int configIx;
  config::Stop stop;


  LoopState(): currentNextStops(3, 0), region_offset(0) {} ;
  void reset(std::vector<int>& nextStops) {
    currentNextStops = nextStops;
    region_offset = 0;

    syms.clear();
    for (auto s : nextStops) {
      if (s == 0) {
        syms.append(*symbol::SYM_NUMERIC[0]);
        syms.append(symbol::SPACE);
        syms.append(symbol::SPACE);
      } else {
        symbol::addNumberToArray(syms, s);
        syms.append(symbol::SPACE);
        syms.append(symbol::SPACE);
      }
    }
  }

  void inc() {
    region_offset = (region_offset + 1) % syms.cols();
    cyclesSinceRequest++;
  }
};


LoopState LOOP_STATE; 
NextStopClient client;

void setup(void) {
  // pinMode(led, OUTPUT);
  // digitalWrite(led, 0);
  Serial.begin(115200);

  if (!LittleFS.begin()) {
    Serial.println("Err: LittleFS mount failed");
    return;
  }

  config::Config conf;
  if (!readConfig("/next_stop.config", &conf)) {
    Serial.println("Err: Read config failed");
    return;
  }

  setupWifi(conf.connection.ssid.c_str(), conf.connection.password.c_str());
  setupLeds();

  LOOP_STATE = LoopState();
  LOOP_STATE.cyclesSinceRequest = IDLE_CYCLES;

  LOOP_STATE.stop.direction = conf.stops[conf.active_stop].direction;
  LOOP_STATE.stop.url = conf.stops[conf.active_stop].url;

  client.setUrl(std::string(LOOP_STATE.stop.url));
}

void loop(void) { 
  if (LOOP_STATE.cyclesSinceRequest >= IDLE_CYCLES) {
    std::vector<int> result(3 ,0);
    int stops = client.getNextStops(result.begin(), result.end());
    
    if (stops == 0) {
      Serial.printf("No new stops, skipping\n");
      delay(1000);
      return;
    }

    LOOP_STATE.cyclesSinceRequest = 0;
    for (int i = 0; i < stops; i++) {
      Serial.printf("In: %d minutes\n", result[i]);
    }

    if (result != LOOP_STATE.currentNextStops) {
      LOOP_STATE.reset(result);
      Serial.printf("Update next stops");
    }
  }

  for (int i=0; i<NUM_LEDS; i++){
    leds[i] = CRGB::Black;
  }

  int col = 0;

  ////////  Line  ////////
  col = drawSymbol(col, symbol::SymbolArray {
    symbol::ONE_TRAIN,
    symbol::SPACE
  }, CRGB::Green, 0);


  ////////  Direction  ////////
  col = drawSymbol(col, symbol::SymbolArray {
    (LOOP_STATE.stop.direction == config::Direction::South) ? symbol::ARROW_DOWN : symbol::ARROW_UP,
    symbol::SPACE
  }, CRGB::Yellow, 0);

  ////////  Minutes  ////////
  int minutesStartCol = col;
  col = drawSymbol(col, LOOP_STATE.syms, CRGB::Red, LOOP_STATE.region_offset, minutesStartCol);

  FastLED.show();
  FastLED.delay(800);
  LOOP_STATE.inc();
}
