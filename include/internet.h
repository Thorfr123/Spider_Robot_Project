#ifndef _INTERNET_H_
#define _INTERNET_H_

#include <Arduino.h>
#include <WiFi.h>

// Comment this line if you want to use the Raspberry as a STA
#define USE_WIFI_AP

#define AP_WIFI_NETWORK     "Pico_W_AccessPoint"
#define AP_WIFI_PASSWORD    "12345678"
#define WIFI_TIMEOUT_MS     20000

// Change these parameters according to your network
#define WIFI_NETWORK        "EXAMPLE-354G"        // SSID of the WiFi Network
#define WIFI_PASSWORD       "1234567"        	  // Password of the WiFi Network


void createAccessPoint();

void connectToStation();

void connectToWiFi();

void printHTML(WiFiClient& client);

#endif