#ifndef STOCKS_CONFIG_H
#define STOCKS_CONFIG_H

// Replace this with your Google Apps Script Web App URL
const char* stockScriptURL = "https://script.google.com/macros/s/AKfycbxwJJIljzGWv9tVyVsslS33cr78anSbfUun2JMa-u2QIu2GDqwZqtEiq7t791kHyk-3/exec";

// Stock Data Variables
float portfolioBalance = 0.0;
float portfolioChange  = 0.0;
float portfolioProfit  = 0.0;
int   daysActive       = 0;

// Watchlist Data Structure
struct Asset {
  String symbol;
  float price;
};

Asset watchlist[5]; // Array to store the 5 assets from your sheet
int assetCount = 0; // This was the missing variable

// Update interval (5 minutes)
const unsigned long STOCK_INTERVAL = 300000;
unsigned long lastStockUpdate = 0;

#endif
