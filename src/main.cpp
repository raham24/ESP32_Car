#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>

#include "config.h"
#include "secrets.h"
#include "drive.h"
#include "web_ui.h"

static WebServer server(HTTP_PORT);

// ---------------------------------------------------------------------------
// WiFi
// ---------------------------------------------------------------------------

static void connectWiFi() {
  pinMode(STATUS_LED_PIN, OUTPUT);

  WiFi.mode(WIFI_STA);
  WiFi.setHostname(CAR_HOSTNAME);

#if WIFI_ENTERPRISE
  // WPA2-Enterprise (PEAP/MSCHAPv2): username + password login
  WiFi.begin(WIFI_SSID, WPA2_AUTH_PEAP, WIFI_EAP_IDENTITY, WIFI_EAP_USERNAME, WIFI_EAP_PASSWORD);
  Serial.printf("[wifi] connecting to \"%s\" (enterprise, user %s)", WIFI_SSID, WIFI_EAP_USERNAME);
#else
  // Regular WPA/WPA2: password only
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("[wifi] connecting to \"%s\"", WIFI_SSID);
#endif

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));  // blink while connecting
    delay(250);
    Serial.print('.');
    if (millis() - start > WIFI_CONNECT_TIMEOUT_MS) {
      Serial.println("\n[wifi] timed out, rebooting");
      ESP.restart();
    }
  }

  digitalWrite(STATUS_LED_PIN, HIGH);
  Serial.printf("\n[wifi] connected, IP: %s  RSSI: %d dBm\n",
                WiFi.localIP().toString().c_str(), WiFi.RSSI());

  if (MDNS.begin(CAR_HOSTNAME)) {
    MDNS.addService("http", "tcp", HTTP_PORT);
    Serial.printf("[mdns] reachable at http://%s.local\n", CAR_HOSTNAME);
  } else {
    Serial.println("[mdns] failed to start");
  }
}

// ---------------------------------------------------------------------------
// HTTP helpers
// ---------------------------------------------------------------------------

static void sendJson(int code, const JsonDocument& doc) {
  String out;
  serializeJson(doc, out);
  server.send(code, "application/json", out);
}

static void sendError(int code, const char* message) {
  JsonDocument doc;
  doc["ok"] = false;
  doc["error"] = message;
  sendJson(code, doc);
}

// ---------------------------------------------------------------------------
// Routes
// ---------------------------------------------------------------------------

// GET /  -> control page
static void handleRoot() {
  server.send_P(200, "text/html", WEB_UI);
}

// GET /status -> current state + connection info
static void handleStatus() {
  const DriveState& s = driveGet();
  JsonDocument doc;
  doc["ok"] = true;
  doc["ip"] = WiFi.localIP().toString();
  doc["rssi"] = WiFi.RSSI();
  doc["uptimeMs"] = millis();
  doc["throttle"] = s.throttle;
  doc["steering"] = s.steering;
  doc["failsafe"] = s.failsafeActive;
  sendJson(200, doc);
}

// POST /drive  body: {"throttle": -100..100, "steering": -100..100}
static void handleDrive() {
  if (!server.hasArg("plain")) {
    sendError(400, "missing JSON body");
    return;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, server.arg("plain"));
  if (err) {
    sendError(400, "invalid JSON");
    return;
  }
  if (!doc["throttle"].is<int>() || !doc["steering"].is<int>()) {
    sendError(400, "throttle and steering must be integers");
    return;
  }

  driveSet(doc["throttle"].as<int>(), doc["steering"].as<int>());

  JsonDocument res;
  res["ok"] = true;
  res["throttle"] = driveGet().throttle;
  res["steering"] = driveGet().steering;
  sendJson(200, res);
}

// POST /stop
static void handleStop() {
  driveStop();
  JsonDocument doc;
  doc["ok"] = true;
  sendJson(200, doc);
}

static void handleNotFound() {
  sendError(404, "not found");
}

static void setupServer() {
  // Allow browser-based apps served from elsewhere (e.g. a dev server) to call the API
  server.enableCORS(true);

  server.on("/", HTTP_GET, handleRoot);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/drive", HTTP_POST, handleDrive);
  server.on("/stop", HTTP_POST, handleStop);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.printf("[http] server listening on port %d\n", HTTP_PORT);
}

// ---------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n=== ESP32 Car ===");

  connectWiFi();
  setupServer();
}

void loop() {
  server.handleClient();
  driveUpdate();

  // Reconnect if WiFi drops
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[wifi] connection lost, reconnecting");
    driveStop();
    connectWiFi();
  }
}
