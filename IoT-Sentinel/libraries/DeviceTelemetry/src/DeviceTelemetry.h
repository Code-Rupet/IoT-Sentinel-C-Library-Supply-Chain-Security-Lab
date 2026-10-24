#ifndef DEVICE_TELEMETRY_H
#define DEVICE_TELEMETRY_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

/**
 * ============================================================================
 * DeviceTelemetry — Research & Production Telemetry Engine
 * ============================================================================
 * 
 * TELEMETRY SPECIFICATION & PRIVACY DISCLOSURE:
 * --------------------------------------------
 * 1. Fields Collected:
 *    - `device_id`        : Unique identifier for node tracking (e.g. "esp32-sentinel-01").
 *    - `event`            : Trigger type ("startup", "test", "heartbeat", "led_change", "simulated_anomaly").
 *    - `firmware_version` : Active firmware version for fleet management.
 *    - `uptime_seconds`   : Monotonic counter derived from millis() for stability audits.
 *    - `wifi_connected`   : Network status flag.
 *    - `wifi_rssi`        : Signal strength in dBm to monitor RF propagation.
 *    - `led_state`        : Operational state of the hardware actuator ("ON" / "OFF").
 * 
 * 2. Why Collected:
 *    - Necessary to verify device operational health, diagnose RF packet loss, and validate remote state.
 * 
 * 3. Network Destination:
 *    - Configured Google Apps Script Web App receiver (HTTPS on TCP port 443).
 *    - Server certificate validated against Google Trust Services (GTS) Root R1 CA.
 * 
 * 4. Transmission Triggers:
 *    - System boot ("startup")
 *    - Post-connection verification ("test")
 *    - Periodic keepalive timer (every 60 seconds: "heartbeat")
 *    - Hardware state changes ("led_change")
 *    - Controlled research simulation ("simulated_anomaly" in Configuration C)
 * ============================================================================
 */

struct TelemetryResult {
  bool success;
  int httpCode;
  String responseBody;
  String errorMessage;
};

class DeviceTelemetry {
public:
  DeviceTelemetry();

  /**
   * Initializes the telemetry engine with network destinations and security roots.
   *
   * @param deviceId        Unique device identifier
   * @param firmwareVersion Semantic firmware version string
   * @param scriptUrl       Target Google Apps Script Web App HTTPS URL
   * @param authToken       Shared secret authentication token
   * @param caCertPem       Root CA certificate in PEM format for TLS verification
   */
  void begin(const char* deviceId,
             const char* firmwareVersion,
             const char* scriptUrl,
             const char* authToken,
             const char* caCertPem);

  /**
   * Transmits an approved operational telemetry event.
   */
  TelemetryResult sendEvent(const char* eventType,
                            bool wifiConnected,
                            int rssi,
                            bool ledState);

  /**
   * Configuration C Simulation:
   * Emits a synthetic, clearly labeled anomalous telemetry record representing
   * unexpected third-party library behavior in a controlled analysis pipeline.
   * Does NOT collect or exfiltrate private credentials.
   */
  TelemetryResult sendSimulatedAnomaly(bool wifiConnected,
                                       int rssi,
                                       bool ledState,
                                       const char* syntheticReason);

  /**
   * Evaluates if a redirection target points strictly to Google infrastructure.
   */
  static bool isSafeGoogleRedirect(const String& url);

  /**
   * Utility to compute monotonic uptime in seconds.
   */
  static unsigned long getUptimeSeconds();

private:
  const char* _deviceId;
  const char* _firmwareVersion;
  const char* _scriptUrl;
  const char* _authToken;
  const char* _caCertPem;
  WiFiClientSecure _secureClient;

  TelemetryResult executePost(const String& url, const String& payload);
  TelemetryResult executeGet(const String& url);
  void logSanitized(const char* prefix, const String& content);
};

#endif // DEVICE_TELEMETRY_H
