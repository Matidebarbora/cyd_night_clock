#ifndef CYD_CONFIG_H
#define CYD_CONFIG_H

// ===== TFT_eSPI Configuration for ESP32-2432S028 (CYD) =====
#define USER_SETUP_INFO "CYD_Config"
#define USER_SETUP_LOADED

// Driver selection
#define ILI9341_2_DRIVER

// Display dimensions
#define TFT_WIDTH  240
#define TFT_HEIGHT 320

// Pin definitions for ESP32-2432S028 (CYD)
#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC   2
#define TFT_RST  -1

#define TFT_BL   21
#define TFT_BACKLIGHT_ON HIGH

#define TOUCH_CS 33

// Fonts to load
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF

#define SMOOTH_FONT

// SPI settings
#define SPI_FREQUENCY  55000000
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  2500000

#define USE_HSPI_PORT

#endif // CYD_CONFIG_H
