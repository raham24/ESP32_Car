#pragma once

// Hostname used for mDNS: the car is reachable at http://esp32car.local
#define CAR_HOSTNAME "esp32car"

// HTTP server port
#define HTTP_PORT 80

// Failsafe: if no drive command arrives within this window, the car stops.
// The app should send commands at least ~5x faster than this (e.g. every 100 ms).
#define FAILSAFE_TIMEOUT_MS 500

// How long to wait for WiFi before rebooting and trying again
#define WIFI_CONNECT_TIMEOUT_MS 20000

// Onboard LED (GPIO 2 on most ESP32 dev boards) - blinks while connecting, solid when online
#define STATUS_LED_PIN 2
