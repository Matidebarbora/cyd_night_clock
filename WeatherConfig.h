#ifndef WEATHER_CONFIG_H
#define WEATHER_CONFIG_H

// Quilicura, Santiago, Chile
const float LATITUDE  = -33.36; 
const float LONGITUDE = -70.73;

// API Endpoint for Open-Meteo
const char* weatherEndpoint = "http://api.open-meteo.com/v1/forecast?latitude=-33.36&longitude=-70.73&current=temperature_2m,weather_code&daily=weather_code,temperature_2m_max,temperature_2m_min&timezone=auto";

// Weather Code Translator (Simplified)
String getWeatherDesc(int code) {
  if (code == 0) return "Clear Sky";
  if (code <= 3) return "Cloudy";
  if (code <= 48) return "Foggy";
  if (code <= 67) return "Rainy";
  if (code <= 77) return "Snowy";
  if (code >= 80) return "Showers";
  return "Unknown";
}

#endif
