#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

/**
 * ============================================================================
 * IoT Sentinel — Research Configuration & Hardware Definitions
 * ============================================================================
 */

// --- Secrets Inclusion ---
#if __has_include("secrets.h")
  #include "secrets.h"
#else
  #warning "'secrets.h' not found. Falling back to 'secrets.example.h'. Ensure you create 'secrets.h' with real credentials!"
  #include "secrets.example.h"
#endif

// ============================================================================
// EXPERIMENT CONFIGURATION SELECTOR
// ============================================================================
// Set EXPERIMENT_CONFIG to one of the following research configurations:
//   CONFIG_A : Baseline (LED actuator active, Wi-Fi connected, ZERO telemetry)
//   CONFIG_B : Approved Telemetry (LED + Documented DeviceTelemetry keepalive)
//   CONFIG_C : Simulated Compromised Dependency (Controlled synthetic anomaly)
// ============================================================================
#define CONFIG_A 1  // Baseline (No telemetry)
#define CONFIG_B 2  // Approved Telemetry
#define CONFIG_C 3  // Simulated Compromised Dependency

#ifndef EXPERIMENT_CONFIG
  #define EXPERIMENT_CONFIG CONFIG_B
#endif

// --- Hardware Pin Definitions ---
// GPIO 2 is standard onboard LED for ESP32 DevKit v1 / NodeMCU-32S.
// Change to 8 for ESP32-C3, or 21/48 for ESP32-S3 boards.
#ifndef STATUS_LED_PIN
  #define STATUS_LED_PIN 2
#endif

#define STATUS_LED_ACTIVE_HIGH true

// --- Device Metadata ---
#define DEVICE_ID        "esp32-sentinel-01"
#define FIRMWARE_VERSION "1.1.0-research"

// --- Telemetry Timing Parameters ---
#define SERIAL_BAUD_RATE            115200
#define HEARTBEAT_INTERVAL_MS       60000UL   // 60 seconds periodic interval
#define WIFI_RECONNECT_INTERVAL_MS  5000UL    // 5 seconds between reconnect checks
#define HTTP_TIMEOUT_MS             15000     // 15 seconds HTTPS client timeout

// --- Time Synchronization (NTP) ---
// Accurate time is strictly mandatory for TLS X.509 certificate validation.
#define NTP_SERVER_PRIMARY   "pool.ntp.org"
#define NTP_SERVER_SECONDARY "time.google.com"
#define NTP_SYNC_TIMEOUT_SEC 15
#define GMT_OFFSET_SEC       0
#define DAYLIGHT_OFFSET_SEC  0

// --- Google Trust Services (GTS) Root R1 CA Certificate ---
// Subject: CN=GTS Root R1, O=Google Trust Services LLC, C=US
// Valid until: Jun 22 00:00:00 2036 GMT
const char GOOGLE_GTS_ROOT_CA[] PROGMEM = 
"-----BEGIN CERTIFICATE-----\n"
"MIIFVzCCAz+gAwIBAgINAgPlk28xsBNJiGuiFzANBgkqhkiG9w0BAQwFADBHMQsw\n"
"CQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExMQzEU\n"
"MBIGA1UEAxMLR1RTIFJvb3QgUjEwHhcNMTYwNjIyMDAwMDAwWhcNMzYwNjIyMDAw\n"
"MDAwWjBHMQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZp\n"
"Y2VzIExMQzEUMBIGA1UEAxMLR1RTIFJvb3QgUjEwggIiMA0GCSqGSIb3DQEBAQUA\n"
"A4ICDwAwggIKAoICAQC2EQKLHuOhd5s73L+UPreVp0A8of2C+X0yBoJx9vaMf/vo\n"
"27xqLpeXo4xL+Sv2sfnOhB2x+cWX3u+58qPpvBKJXqeqUqv4IyfLpLGcY9vXmX7w\n"
"Cl7raKb0xlpHDU0QM+NOsROjyBhsS+z8CZDfnWQpJSMHobTSPS5g4M/SCYe7zUjw\n"
"TcLCeoiKu7rPWRnWr4+wB7CeMfGCwcDfLqZtbBkOtdh+JhpFAz2weaSUKK0Pfybl\n"
"qAj+lug8aJRT7oM6iCsVlgmy4HqMLnXWnOunVmSPlk9orj2XwoSPwLxAwAtcvfaH\n"
"szVsrBhQf4TgTM2S0yDpM7xSma8ytSmzJSq0SPly4cpk9+aCEI3oncKKiPo4Zor8\n"
"Y/kB+Xj9e1x3+naH+uzfsQ55lVe0vSbv1gHR6xYKu44LtcXFilWr06zqkUspzBmk\n"
"MiVOKvFlRNACzqrOSbTqn3yDsEB750Orp2yjj32JgfpMpf/VjsPOS+C12LOORc92\n"
"wO1AK/1TD7Cn1TsNsYqiA94xrcx36m97PtbfkSIS5r762DL8EGMUUXLeXdYWk70p\n"
"aDPvOmbsB4om3xPXV2V4J95eSRQAogB/mqghtqmxlbCluQ0WEdrHbEg8QOB+DVrN\n"
"VjzRlwW5y0vtOUucxD/SVRNuJLDWcfr0wbrM7Rv1/oFB2ACYPTrIrnqYNxgFlQID\n"
"AQABo0IwQDAOBgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4E\n"
"FgQU5K8rJnEaK0gnhS9SZizv8IkTcT4wDQYJKoZIhvcNAQEMBQADggIBAJ+qQibb\n"
"C5u+/x6Wki4+omVKapi6Ist9wTrYggoGxval3sBOh2Z5ofmmWJyq+bXmYOfg6LEe\n"
"QkEzCzc9zolwFcq1JKjPa7XSQCGYzyI0zzvFIoTgxQ6KfF2I5DUkzps+GlQebtuy\n"
"h6f88/qBVRRiClmpIgUxPoLW7ttXNLwzldMXG+gnoot7TiYaelpkttGsN/H9oPM4\n"
"7HLwEXWdyzRSjeZ2axfG34arJ45JK3VmgRAhpuo+9K4l/3wV3s6MJT/KYnAK9y8J\n"
"ZgfIPxz88NtFMN9iiMG1D53Dn0reWVlHxYciNuaCp+0KueIHoI17eko8cdLiA6Ef\n"
"MgfdG+RCzgwARWGAtQsgWSl4vflVy2PFPEz0tv/bal8xa5meLMFrUKTX5hgUvYU/\n"
"Z6tGn6D/Qqc6f1zLXbBwHSs09dR2CQzreExZBfMzQsNhFRAbd03OIozUhfJFfbdT\n"
"6u9AWpQKXCBfTkBdYiJ23//OYb2MI3jSNwLgjt7RETeJ9r/tSQdirpLsQBqvFAnZ\n"
"0E6yove+7u7Y/9waLd64NnHi/Hm3lCXRSHNboTXns5lndcEZOitHTtNCjv0xyBZm\n"
"2tIMPNuzjsmhDYAPexZ3FL//2wmUspO8IFgV6dtxQ/PeEMMA3KgqlbbC1j+Qa3bb\n"
"bP6MvPJwNQzcmRk13NfIRmPVNnGuV/u3gm3c\n"
"-----END CERTIFICATE-----\n";

#endif // CONFIG_H
