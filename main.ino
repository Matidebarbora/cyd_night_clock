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

// 1. Initialize Display and Colors First
TFT_eSPI tft = TFT_eSPI();

#define COLOR_BG      0x2104 
#define COLOR_ACCENT  0xFDA0 
#define COLOR_TEXT    0xFFFF 
#define COLOR_HEADER  0x0000 
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0

// 2. Include Stocks Configuration after colors are defined
#include "StocksConfig.h" 

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
enum ScreenState { MAIN, WEATHER, STOCKS };
ScreenState currentScreen = MAIN;

String currentTime = "00:00:00";
String currentDate = "00-00-0000";
String weatherDesc = "Updating...";
float mainTemp = 0, mainMin = 0, mainMax = 0;

struct Forecast { float min; float max; };
Forecast weekly[3]; 

unsigned long lastTimeUpdate = 0;
unsigned long lastWeatherUpdate = 0;
const unsigned long WEATHER_INTERVAL = 900000; // 15 mins

// ===================================================
// 1. Data Logic
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

void fetchStocks() {
  if (WiFi.status() != WL_CONNECTED) return;
  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.begin(stockScriptURL);

  http.setTimeout(20000);
  
  int httpCode = http.GET();
  Serial.print("Stock HTTP Code: ");
  Serial.println(httpCode);

  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println("Received Payload: " + payload); // Check if this looks like JSON

    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
      Serial.print("JSON Error: ");
      Serial.println(error.f_str());
      return;
    }
    
    portfolioBalance = doc["balance"];
    portfolioChange  = doc["change"];
    portfolioProfit  = doc["profit"];
    daysActive       = doc["active"];

    JsonArray arr = doc["watchlist"];
    assetCount = 0; 
    for (JsonObject obj : arr) {
      if (assetCount < 5) {
        watchlist[assetCount].symbol = obj["s"].as<String>();
        watchlist[assetCount].price  = obj["p"].as<float>();
        assetCount++;
      }
    }
    Serial.println("Stock Data Parsed Successfully");
  } else {
    Serial.printf("HTTP Error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}

// ===================================================
// 2. UI Drawing Helpers
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
  tft.setTextColor(active == STOCKS ? COLOR_ACCENT : COLOR_TEXT);
  tft.drawString("STOCKS", 267, FOOTER_Y + 20);
}

void drawMainScreen() {
  tft.fillScreen(COLOR_BG);
  tft.fillRect(0, 0, 320, 70, COLOR_HEADER);
  tft.setTextSize(1);
  tft.setTextColor(COLOR_ACCENT, COLOR_HEADER); 
  tft.setTextDatum(MC_DATUM);
  tft.drawString(currentTime, 160, 35, 7); 

  tft.setTextSize(2); 
  tft.setTextColor(COLOR_ACCENT, COLOR_BG);
  tft.drawString(currentDate, 160, 95);
  tft.drawString(weatherDesc, 160, 125);

  tft.setTextColor(COLOR_ACCENT);
  tft.drawString("Min", 60, 155);
  tft.drawString("Current", 160, 155);
  tft.drawString("Max", 260, 155);

  tft.setTextColor(COLOR_TEXT);
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

void drawStocksScreen() {
  tft.fillScreen(COLOR_BG);
  tft.fillRect(0, 0, 320, 35, COLOR_HEADER);
  tft.setTextSize(1); 
  tft.setTextColor(COLOR_ACCENT); 
  //tft.setTextDatum(MC_DATUM);
  //tft.drawString("STOCKS WATCHLIST", 160, 17);
  tft.drawString("BALANCE: $" + String(portfolioBalance, 2), 65, 17, 2);
  tft.setTextColor(portfolioChange >= 0 ? COLOR_GREEN : COLOR_RED);
  tft.drawRightString(String(portfolioChange, 2) + "%", 270, 9, 2); 

  // Summary Row - Using Font 2 explicitly to ensure visibility
  // tft.setTextDatum(TL_DATUM);
  // tft.setTextSize(1); 
  // tft.setTextColor(COLOR_TEXT);
  // tft.drawString("Balance: $" + String(portfolioBalance, 2), 10, 45, 2); 
  
  // Percent Change
  // tft.setTextColor(portfolioChange >= 0 ? COLOR_GREEN : COLOR_RED);
  // tft.drawRightString(String(portfolioChange, 2) + "%", 310, 45, 2);

  // Table Header
  tft.drawLine(0, 40, 320, 40, COLOR_ACCENT); //   tft.drawLine(0, 65, 320, 65, COLOR_ACCENT);
  tft.setTextColor(COLOR_ACCENT);
  tft.drawString("SYMBOL", 50, 52, 2); //   tft.drawString("SYMBOL", 50, 52, 2);
  tft.drawString("PRICE", 280, 52, 2);
  tft.drawLine(0, 65, 320, 65, COLOR_ACCENT); //   tft.drawLine(0, 90, 320, 90, COLOR_ACCENT);

  // Asset Rows
  for (int i = 0; i < assetCount; i++) {
    int yPos = 75 + (i * 22); 
    
    // Symbol in Accent Color (Font 2, Size 1)
    tft.setTextColor(COLOR_ACCENT);
    tft.drawString(watchlist[i].symbol, 50, yPos, 2);
    
    // Price in White (Font 2, Size 1)
    tft.setTextColor(COLOR_TEXT);
    String priceStr = "$" + String(watchlist[i].price, 2);
    tft.drawRightString(priceStr, 300, yPos, 2); 
  }

  drawWifiDot();
  drawFooter(STOCKS);
}

// ===================================================
// 3. Main Loop Logic
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
  fetchStocks();
  
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
      else if (tx >= 213 && currentScreen != STOCKS) { currentScreen = STOCKS; drawStocksScreen(); }
      delay(200); 
    }
  }

  // Update Clock every second
  if (millis() - lastTimeUpdate > 1000) {
    struct tm ti;
    if (getLocalTime(&ti)) {
      char ts[10], ds[12];
      strftime(ts, sizeof(ts), "%H:%M:%S", &ti);
      strftime(ds, sizeof(ds), "%d-%m-%Y", &ti);
      currentTime = String(ts);
      currentDate = String(ds);
      
      if (currentScreen == MAIN) {
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

  // Update Stocks every 5 mins
  if (millis() - lastStockUpdate > STOCK_INTERVAL) {
    fetchStocks();
    if (currentScreen == STOCKS) drawStocksScreen();
    lastStockUpdate = millis();
  }
}
