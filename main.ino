#include "CYD_Config.h" 
#include "WifiConfig.h" 
#include "WeatherConfig.h"
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "time.h"

TFT_eSPI tft = TFT_eSPI();

// ===== COLOR CONFIGURATION =====
#define COLOR_BG      0x2104 
#define COLOR_ACCENT  0xFDA0 
#define COLOR_TEXT    0xFFFF 
#define COLOR_HEADER  0x0000 

// ===== Peripherals =====
#define XPT2046_IRQ  36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK  25
#define XPT2046_CS   33

SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

// ===== UI State =====
const int FOOTER_Y = 200;
enum ScreenState { MAIN, WEATHER, CONFIG };
ScreenState currentScreen = MAIN;

String currentTime = "00:00:00";
String currentDate = "00-00-0000";
String weatherDesc = "Updating...";
float mainTemp = 0, mainMin = 0, mainMax = 0;

struct Forecast { float min; float max; };
Forecast weekly[3]; 

unsigned long lastTimeUpdate = 0;
unsigned long lastWeatherUpdate = 0;
const unsigned long WEATHER_INTERVAL = 900000; 

// ===================================================
// 1. Helper Logic
// ===================================================

void drawWifiDot() {
  uint16_t dotColor = (WiFi.status() == WL_CONNECTED) ? TFT_GREEN : TFT_RED;
  tft.fillCircle(310, 10, 4, dotColor);
}

void connectToWifi() {
  tft.fillScreen(COLOR_BG);
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(2);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("Connecting WiFi...", 160, 100);

  for (int i = 0; i < networkCount; i++) {
    WiFi.begin(myNetworks[i].ssid, myNetworks[i].password);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 15) {
      delay(500);
      attempts++;
    }
    if (WiFi.status() == WL_CONNECTED) {
      tft.drawString("Connected!", 160, 140);
      delay(1000);
      return;
    }
  }
}

void fetchWeather() {
  if (WiFi.status() != WL_CONNECTED) return;
  HTTPClient http;
  http.begin(weatherEndpoint);
  int httpCode = http.GET();
  if (httpCode > 0) {
    String payload = http.getString();
    StaticJsonDocument<1500> doc;
    deserializeJson(doc, payload);
    mainTemp = doc["current"]["temperature_2m"];
    weatherDesc = getWeatherDesc(doc["current"]["weather_code"]);
    for (int i = 0; i < 3; i++) {
      weekly[i].min = doc["daily"]["temperature_2m_min"][i];
      weekly[i].max = doc["daily"]["temperature_2m_max"][i];
    }
    mainMin = weekly[0].min; mainMax = weekly[0].max;
  }
  http.end();
}

// ===================================================
// 2. UI Screens
// ===================================================

void drawFooter(ScreenState active) {
  tft.fillRect(0, FOOTER_Y, 320, 40, COLOR_HEADER);
  tft.drawLine(0, FOOTER_Y, 320, FOOTER_Y, COLOR_BG);
  tft.setTextSize(2);
  tft.setTextDatum(MC_DATUM);

  tft.setTextColor(active == MAIN ? COLOR_ACCENT : COLOR_TEXT);
  tft.drawString("HOME", 53, FOOTER_Y + 20);
  tft.setTextColor(active == WEATHER ? COLOR_ACCENT : COLOR_TEXT);
  tft.drawString("WEATHER", 160, FOOTER_Y + 20);
  tft.setTextColor(active == CONFIG ? COLOR_ACCENT : COLOR_TEXT);
  tft.drawString("CONFIG", 267, FOOTER_Y + 20);
}

void drawMainScreen() {
  tft.fillScreen(COLOR_BG);
  tft.fillRect(0, 0, 320, 70, COLOR_HEADER);
  
  // Time (Anti-Flicker: Print with background color set)
  tft.setTextSize(1);
  tft.setTextColor(COLOR_ACCENT, COLOR_HEADER); 
  tft.setTextDatum(MC_DATUM);
  tft.drawString(currentTime, 160, 35, 7); 

  tft.setTextSize(2); 
  tft.setTextColor(COLOR_ACCENT, COLOR_BG);
  tft.drawString(currentDate, 160, 95);
  tft.drawString(weatherDesc, 160, 125);

  // Swapped Colors as requested
  tft.setTextColor(COLOR_ACCENT); // Labels Amber
  tft.drawString("Min", 60, 155);
  tft.drawString("Current", 160, 155);
  tft.drawString("Max", 260, 155);

  tft.setTextColor(COLOR_TEXT);   // Values White
  tft.drawString(String((int)mainMin) + "C", 60, 180);
  tft.drawString(String((int)mainTemp) + "C", 160, 180);
  tft.drawString(String((int)mainMax) + "C", 260, 180);

  drawWifiDot();
  drawFooter(MAIN);
}

void drawWeatherScreen() {
  tft.fillScreen(COLOR_BG);
  tft.fillRect(0, 0, 320, 40, COLOR_HEADER);
  tft.setTextSize(2);
  tft.setTextColor(COLOR_ACCENT);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("WEATHER FORECAST", 160, 20);

  tft.setTextDatum(TL_DATUM);
  tft.setCursor(150, 55); tft.print("MIN");
  tft.setCursor(240, 55); tft.print("MAX");

  int startY = 90;
  String labels[] = {"TODAY", "TOMORROW", "DAY AFTER T"};
  for(int i = 0; i < 3; i++) {
    tft.setTextColor(COLOR_ACCENT);
    tft.setCursor(10, startY + (i * 35)); tft.print(labels[i]);
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(150, startY + (i * 35)); tft.print(String((int)weekly[i].min) + "C");
    tft.setCursor(240, startY + (i * 35)); tft.print(String((int)weekly[i].max) + "C");
  }
  drawWifiDot();
  drawFooter(WEATHER);
}

void drawConfigScreen() {
  tft.fillScreen(COLOR_BG);
  tft.fillRect(0, 0, 320, 40, COLOR_HEADER);
  tft.setTextSize(2);
  tft.setTextColor(COLOR_ACCENT);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("CONFIGURATION", 160, 20);

  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(2);
  tft.setCursor(20, 80);
  tft.print("WiFi SSID:");
  tft.setTextColor(COLOR_ACCENT);
  tft.setCursor(20, 110);
  tft.print(WiFi.SSID());

  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(20, 150);
  tft.print("IP Address:");
  tft.setCursor(20, 180);
  tft.print(WiFi.localIP().toString());

  drawWifiDot();
  drawFooter(CONFIG);
}

// ===================================================
// 3. Main Logic
// ===================================================

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(1);
  
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);
  touchscreen.setRotation(1);

  connectToWifi();
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  fetchWeather(); 
  drawMainScreen();
}

void loop() {
  if (touchscreen.touched()) {
    TS_Point p = touchscreen.getPoint();
    int tx = map(p.x, 200, 3700, 0, 320);
    int ty = map(p.y, 240, 3800, 0, 240);
    if (ty > FOOTER_Y) {
      if (tx < 106 && currentScreen != MAIN) { currentScreen = MAIN; drawMainScreen(); }
      else if (tx >= 106 && tx < 213 && currentScreen != WEATHER) { currentScreen = WEATHER; drawWeatherScreen(); }
      else if (tx >= 213 && currentScreen != CONFIG) { currentScreen = CONFIG; drawConfigScreen(); }
      delay(200); 
    }
  }

  // Update Clock every second
  if (millis() - lastTimeUpdate > 1000) {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      char tStr[10], dStr[12];
      strftime(tStr, sizeof(tStr), "%H:%M:%S", &timeinfo);
      strftime(dStr, sizeof(dStr), "%d-%m-%Y", &timeinfo);
      currentTime = String(tStr);
      currentDate = String(dStr);
      
      if (currentScreen == MAIN) {
        // No fillRect here = No Flicker. 
        // We print the text with its background color to "self-erase" old digits.
        tft.setTextSize(1);
        tft.setTextColor(COLOR_ACCENT, COLOR_HEADER); 
        tft.setTextDatum(MC_DATUM);
        tft.drawString(currentTime, 160, 35, 7);
      }
    }
    lastTimeUpdate = millis();
  }

  // Update Weather every 15 mins
  if (millis() - lastWeatherUpdate > WEATHER_INTERVAL) {
    fetchWeather();
    if (currentScreen == MAIN) drawMainScreen();
    lastWeatherUpdate = millis();
  }
}
