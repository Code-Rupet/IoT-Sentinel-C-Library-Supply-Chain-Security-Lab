/**
 * ============================================================================
 * IoT Sentinel — C++ Library Supply-Chain Security Research Node
 * ============================================================================
 * Investigates the research question:
 * "Can a third-party C++ library introduce network communication or runtime 
 *  behavior that differs from what a developer expects based on the main 
 *  application's source code?"
 *
 * Controlled Configurations:
 *  - CONFIG_A : Baseline (SmartLED active, zero network telemetry)
 *  - CONFIG_B : Approved Telemetry (SmartLED + Documented DeviceTelemetry)
 *  - CONFIG_C : Simulated Compromised Dependency (Controlled synthetic anomaly)
 * ============================================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include "config.h"

// --- Third-Party / Modular C++ Libraries ---
#if __has_include(<SmartLED.h>)
  #include <SmartLED.h>
#else
  #include "../../libraries/SmartLED/src/SmartLED.h"
#endif

#if (EXPERIMENT_CONFIG == CONFIG_B) || (EXPERIMENT_CONFIG == CONFIG_C)
  #if __has_include(<DeviceTelemetry.h>)
    #include <DeviceTelemetry.h>
  #else
    #include "../../libraries/DeviceTelemetry/src/DeviceTelemetry.h"
  #endif
#endif

// --- Hardware & Library Instances ---
SmartLED ledIndicator(STATUS_LED_PIN, STATUS_LED_ACTIVE_HIGH);

#if (EXPERIMENT_CONFIG == CONFIG_B) || (EXPERIMENT_CONFIG == CONFIG_C)
  DeviceTelemetry telemetryClient;
#endif

// --- Timing & State Variables ---
unsigned long g_lastHeartbeatMs    = 0;
unsigned long g_lastReconnectCheck = 0;
bool          g_wasConnected       = false;

// --- Forward Declarations ---
void connectWiFi();
void maintainWiFi();
void syncClock();
void handleSerialCommands();
void printBanner();
void printHelp();

#if (EXPERIMENT_CONFIG == CONFIG_B) || (EXPERIMENT_CONFIG == CONFIG_C)
void dispatchTelemetry(const char* eventType);
#endif

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  delay(1000);
  printBanner();

  // 1. Initialize SmartLED hardware abstraction
  ledIndicator.begin();
  ledIndicator.turnOn();
  Serial.printf("[MainApp] Initialized SmartLED on GPIO %d (State: ON)\n", STATUS_LED_PIN);

  // 2. Establish network connection
  connectWiFi();

  // 3. NTP Clock Synchronization (Mandatory for TLS 1.3 / X.509 verification)
  syncClock();

  // 4. Initialize Telemetry Engine (Configurations B & C only)
#if (EXPERIMENT_CONFIG == CONFIG_B) || (EXPERIMENT_CONFIG == CONFIG_C)
  Serial.println(F("[MainApp] Initializing DeviceTelemetry library..."));
  telemetryClient.begin(DEVICE_ID,
                        FIRMWARE_VERSION,
                        SECRET_GOOGLE_SCRIPT_URL,
                        SECRET_DEVICE_TOKEN,
                        GOOGLE_GTS_ROOT_CA);

  // Dispatch initial startup verification event
  dispatchTelemetry("startup");
#else
  Serial.println(F("[MainApp:CONFIG_A] Baseline mode: Telemetry engine NOT instantiated."));
  Serial.println(F("[MainApp:CONFIG_A] Zero telemetry will be emitted over the network."));
#endif

  g_lastHeartbeatMs = millis();
  printHelp();
}

void loop() {
  // 1. Maintain Wi-Fi link asynchronously
  maintainWiFi();

  // 2. Process interactive serial commands for live laboratory testing
  handleSerialCommands();

  // 3. Periodic Keepalive Routine
  unsigned long now = millis();
  if (now - g_lastHeartbeatMs >= HEARTBEAT_INTERVAL_MS) {
    g_lastHeartbeatMs = now;

#if (EXPERIMENT_CONFIG == CONFIG_B) || (EXPERIMENT_CONFIG == CONFIG_C)
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println(F("\n[MainApp] 60-Second interval reached. Dispatching 'heartbeat'..."));
      dispatchTelemetry("heartbeat");
    }
#else
    Serial.println(F("\n[MainApp:CONFIG_A] 60-Second timer tick. Telemetry suppressed (Baseline mode)."));
#endif
  }

  delay(10);
}

#if (EXPERIMENT_CONFIG == CONFIG_B) || (EXPERIMENT_CONFIG == CONFIG_C)
void dispatchTelemetry(const char* eventType) {
  bool wifiOk = (WiFi.status() == WL_CONNECTED);
  int rssi = wifiOk ? WiFi.RSSI() : -127;
  bool ledState = ledIndicator.isOn();

  TelemetryResult result = telemetryClient.sendEvent(eventType, wifiOk, rssi, ledState);

  if (result.success) {
    Serial.printf("[MainApp] Telemetry event '%s' verified by remote receiver.\n", eventType);
  } else {
    Serial.printf("[MainApp] Telemetry event '%s' failed (HTTP %d): %s\n",
                  eventType, result.httpCode, result.errorMessage.c_str());
  }

  // Under Configuration C: Demonstrate unexpected dependency side-effect
#if (EXPERIMENT_CONFIG == CONFIG_C)
  if (strcmp(eventType, "led_change") == 0) {
    Serial.println(F("\n[CONFIG_C:SIMULATION] Third-party dependency emitting unexpected synthetic telemetry..."));
    telemetryClient.sendSimulatedAnomaly(wifiOk, rssi, ledState,
                                         "UNEXPECTED_DEPENDENCY_SIDE_EFFECT_ON_ACTUATION");
  }
#endif
}
#endif

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  Serial.printf("[MainApp] Connecting to Wi-Fi SSID: \"%s\" (Password: [PROTECTED])...\n", SECRET_WIFI_SSID);
  WiFi.begin(SECRET_WIFI_SSID, SECRET_WIFI_PASSWORD);

  unsigned long startMs = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - startMs < 10000UL)) {
    delay(500);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    g_wasConnected = true;
    Serial.printf("[MainApp] Wi-Fi Connected! IP: %s | RSSI: %d dBm\n",
                  WiFi.localIP().toString().c_str(), WiFi.RSSI());
  } else {
    Serial.println(F("[MainApp:WARN] Initial Wi-Fi connection timed out. Background retry active."));
  }
}

void maintainWiFi() {
  bool currentlyConnected = (WiFi.status() == WL_CONNECTED);
  if (g_wasConnected && !currentlyConnected) {
    Serial.println(F("[MainApp:WARN] Network connection dropped. Reconnecting..."));
    g_wasConnected = false;
  }
  if (!g_wasConnected && currentlyConnected) {
    Serial.printf("[MainApp] Network restored! IP: %s\n", WiFi.localIP().toString().c_str());
    g_wasConnected = true;
  }
  if (!currentlyConnected) {
    unsigned long now = millis();
    if (now - g_lastReconnectCheck >= WIFI_RECONNECT_INTERVAL_MS) {
      g_lastReconnectCheck = now;
      WiFi.disconnect();
      WiFi.reconnect();
    }
  }
}

void syncClock() {
  if (WiFi.status() != WL_CONNECTED) return;
  Serial.printf("[MainApp] Synchronizing system clock via SNTP (%s)...\n", NTP_SERVER_PRIMARY);
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER_PRIMARY, NTP_SERVER_SECONDARY);

  unsigned long start = millis();
  while (millis() - start < (NTP_SYNC_TIMEOUT_SEC * 1000UL)) {
    time_t now = time(nullptr);
    if (now > 1704067200) { // Jan 1 2024
      struct tm ti;
      gmtime_r(&now, &ti);
      char buf[32];
      strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S UTC", &ti);
      Serial.printf("[MainApp] Clock synced: %s\n", buf);
      return;
    }
    delay(500);
  }
  Serial.println(F("[MainApp:WARN] NTP synchronization timed out."));
}

void handleSerialCommands() {
  if (!Serial.available()) return;
  char c = Serial.read();
  if (c == '\r' || c == '\n') return;

  Serial.printf("\n>>> User Command: '%c'\n", c);
  switch (c) {
    case 'l':
    case 'L':
      ledIndicator.toggle();
      Serial.printf("[SmartLED] LED toggled to: %s\n", ledIndicator.isOn() ? "ON" : "OFF");
#if (EXPERIMENT_CONFIG == CONFIG_B) || (EXPERIMENT_CONFIG == CONFIG_C)
      dispatchTelemetry("led_change");
#else
      Serial.println(F("[CONFIG_A] LED toggled. Zero network traffic generated."));
#endif
      break;

    case 't':
    case 'T':
#if (EXPERIMENT_CONFIG == CONFIG_B) || (EXPERIMENT_CONFIG == CONFIG_C)
      Serial.println(F("[MainApp] Dispatching manual 'test' event..."));
      dispatchTelemetry("test");
#else
      Serial.println(F("[CONFIG_A] Telemetry disabled in baseline mode."));
#endif
      break;

    case 'h':
    case 'H':
#if (EXPERIMENT_CONFIG == CONFIG_B) || (EXPERIMENT_CONFIG == CONFIG_C)
      Serial.println(F("[MainApp] Dispatching manual 'heartbeat' event..."));
      dispatchTelemetry("heartbeat");
#else
      Serial.println(F("[CONFIG_A] Heartbeat disabled in baseline mode."));
#endif
      break;

    case 'c':
    case 'C':
#if (EXPERIMENT_CONFIG == CONFIG_C)
      Serial.println(F("[CONFIG_C] Manually triggering synthetic dependency anomaly..."));
      telemetryClient.sendSimulatedAnomaly(WiFi.status() == WL_CONNECTED,
                                           WiFi.RSSI(),
                                           ledIndicator.isOn(),
                                           "MANUAL_LAB_TRIGGER_TEST");
#else
      Serial.println(F("Anomaly simulation is only available when EXPERIMENT_CONFIG is set to CONFIG_C."));
#endif
      break;

    case 's':
    case 'S':
      Serial.println(F("\n--- [DEVICE STATUS REPORT] ---"));
      Serial.printf(" Device ID:        %s\n", DEVICE_ID);
      Serial.printf(" Firmware Version: %s\n", FIRMWARE_VERSION);
#if (EXPERIMENT_CONFIG == CONFIG_A)
      Serial.println(F(" Active Mode:      CONFIG_A (Baseline — Zero Telemetry)"));
#elif (EXPERIMENT_CONFIG == CONFIG_B)
      Serial.println(F(" Active Mode:      CONFIG_B (Approved Telemetry)"));
#elif (EXPERIMENT_CONFIG == CONFIG_C)
      Serial.println(F(" Active Mode:      CONFIG_C (Simulated Compromised Dependency)"));
#endif
      Serial.printf(" Uptime:           %lu seconds\n", millis() / 1000UL);
      Serial.printf(" Wi-Fi Status:     %s (SSID: %s, RSSI: %d dBm)\n",
                    (WiFi.status() == WL_CONNECTED) ? "CONNECTED" : "DISCONNECTED",
                    SECRET_WIFI_SSID, (WiFi.status() == WL_CONNECTED) ? WiFi.RSSI() : -127);
      Serial.printf(" SmartLED State:   %s (GPIO %d)\n", ledIndicator.isOn() ? "ON" : "OFF", STATUS_LED_PIN);
      Serial.println(F("------------------------------\n"));
      break;

    case '?':
      printHelp();
      break;

    default:
      Serial.printf("Unknown command '%c'. Type '?' for options.\n", c);
      break;
  }
}

void printBanner() {
  Serial.println(F("\n======================================================="));
  Serial.println(F("   IoT Sentinel — C++ Supply-Chain Security Lab Node   "));
  Serial.printf( "   Device ID: %s | Firmware: v%s\n", DEVICE_ID, FIRMWARE_VERSION);
#if (EXPERIMENT_CONFIG == CONFIG_A)
  Serial.println(F("   EXPERIMENT: [CONFIG_A] Baseline (No Telemetry)"));
#elif (EXPERIMENT_CONFIG == CONFIG_B)
  Serial.println(F("   EXPERIMENT: [CONFIG_B] Approved Telemetry Active"));
#elif (EXPERIMENT_CONFIG == CONFIG_C)
  Serial.println(F("   EXPERIMENT: [CONFIG_C] Simulated Compromised Dependency"));
#endif
  Serial.println(F("=======================================================\n"));
}

void printHelp() {
  Serial.println(F("\n--- [INTERACTIVE LAB TEST COMMANDS] ---"));
  Serial.println(F(" Type single character in Serial Monitor & press Enter:"));
  Serial.println(F("   'l' -> Toggle SmartLED (emits 'led_change' in Config B & C)"));
  Serial.println(F("   't' -> Send manual 'test' telemetry event"));
  Serial.println(F("   'h' -> Send manual 'heartbeat' telemetry event"));
  Serial.println(F("   'c' -> Trigger synthetic anomaly event (Config C only)"));
  Serial.println(F("   's' -> Print live diagnostic status report"));
  Serial.println(F("   '?' -> Show this command menu"));
  Serial.println(F("---------------------------------------\n"));
}
