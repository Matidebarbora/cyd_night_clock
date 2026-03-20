#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

struct WifiCredential {
  const char* ssid;
  const char* password;
};

// Add as many networks as you like here
WifiCredential myNetworks[] = {
  {"Room24", "Luanita24"},
  {"VTR-0663620", "Sc3rwcyrwfqn"},
  {"JMD_5381", "12345678"}
};

const int networkCount = sizeof(myNetworks) / sizeof(myNetworks[0]);

// Timezone settings
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -10800; // Chile is UTC-3 (3 * 3600 = 10800)
const int   daylightOffset_sec = 0; // Set to 3600 during Daylight Savings

#endif
