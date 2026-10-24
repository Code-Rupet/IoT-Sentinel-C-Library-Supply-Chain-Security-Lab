#include "DeviceTelemetry.h"
#include <time.h>

DeviceTelemetry::DeviceTelemetry()
  : _deviceId(""), _firmwareVersion(""), _scriptUrl(""), _authToken(""), _caCertPem(nullptr) {}

void DeviceTelemetry::begin(const char* deviceId,
                            const char* firmwareVersion,
                            const char* scriptUrl,
                            const char* authToken,
                            const char* caCertPem) {
  _deviceId = deviceId;
  _firmwareVersion = firmwareVersion;
  _scriptUrl = scriptUrl;
  _authToken = authToken;
  _caCertPem = caCertPem;

  if (_caCertPem != nullptr) {
    _secureClient.setCACert(_caCertPem);
  }
  _secureClient.setTimeout(15);
}

unsigned long DeviceTelemetry::getUptimeSeconds() {
  return millis() / 1000UL;
}

bool DeviceTelemetry::isSafeGoogleRedirect(const String& url) {
  if (!url.startsWith("https://")) {
    Serial.println(F("[DeviceTelemetry:SECURITY] Insecure (non-HTTPS) redirect rejected."));
    return false;
  }

  int hostStart = 8; // Length of "https://"
  int slashIdx = url.indexOf('/', hostStart);
  String host = (slashIdx == -1) ? url.substring(hostStart) : url.substring(hostStart, slashIdx);
  int colonIdx = host.indexOf(':');
  if (colonIdx != -1) {
    host = host.substring(0, colonIdx);
  }
  host.toLowerCase();

  if (host == "script.google.com" ||
      host == "script.googleusercontent.com" ||
      host.endsWith(".google.com") ||
      host.endsWith(".googleusercontent.com")) {
    return true;
  }

  Serial.printf("[DeviceTelemetry:SECURITY] Untrusted redirect host rejected: %s\n", host.c_str());
  return false;
}

TelemetryResult DeviceTelemetry::sendEvent(const char* eventType,
                                          bool wifiConnected,
                                          int rssi,
                                          bool ledState) {
#if ARDUINOJSON_VERSION_MAJOR >= 7
  JsonDocument doc;
#else
  StaticJsonDocument<512> doc;
#endif

  doc["device_id"]        = _deviceId;
  doc["event"]            = eventType;
  doc["firmware_version"] = _firmwareVersion;
  doc["uptime_seconds"]   = getUptimeSeconds();
  doc["wifi_connected"]   = wifiConnected;
  doc["wifi_rssi"]        = rssi;
  doc["led_state"]        = ledState ? "ON" : "OFF";

  if (_authToken != nullptr && strlen(_authToken) > 0) {
    doc["token"] = _authToken;
  }

  String payload;
  serializeJson(doc, payload);

  Serial.printf("[DeviceTelemetry] Dispatching approved '%s' event (%u bytes)...\n",
                eventType, payload.length());
  return executePost(String(_scriptUrl), payload);
}

TelemetryResult DeviceTelemetry::sendSimulatedAnomaly(bool wifiConnected,
                                                      int rssi,
                                                      bool ledState,
                                                      const char* syntheticReason) {
  Serial.println(F("\n*******************************************************************"));
  Serial.println(F(" [SUPPLY-CHAIN SECURITY EXPERIMENT: CONFIGURATION C ACTIVE]        "));
  Serial.println(F(" Emitting synthetic anomalous telemetry to test detection pipeline "));
  Serial.println(F(" (Controlled, benign simulation: No real credentials exposed)      "));
  Serial.println(F("*******************************************************************"));

#if ARDUINOJSON_VERSION_MAJOR >= 7
  JsonDocument doc;
#else
  StaticJsonDocument<768> doc;
#endif

  // Conforms to base schema but injects synthetic anomalous telemetry markers
  doc["device_id"]               = _deviceId;
  doc["event"]                   = "simulated_anomaly";
  doc["firmware_version"]        = _firmwareVersion;
  doc["uptime_seconds"]          = getUptimeSeconds();
  doc["wifi_connected"]          = wifiConnected;
  doc["wifi_rssi"]               = rssi;
  doc["led_state"]               = ledState ? "ON" : "OFF";
  
  // Synthetic anomaly payload fields for analysis pipeline detection
  doc["synthetic_anomaly_flag"]  = true;
  doc["synthetic_reason"]        = syntheticReason;
  doc["synthetic_extra_metric"]  = "RESEARCH_SIMULATION_UNAPPROVED_LIBRARY_BEHAVIOR";

  if (_authToken != nullptr && strlen(_authToken) > 0) {
    doc["token"] = _authToken;
  }

  String payload;
  serializeJson(doc, payload);

  return executePost(String(_scriptUrl), payload);
}

TelemetryResult DeviceTelemetry::executePost(const String& url, const String& payload) {
  TelemetryResult result;
  result.success = false;
  result.httpCode = 0;

  time_t now = time(nullptr);
  if (now < 1704067200) {
    Serial.println(F("[DeviceTelemetry:WARN] NTP clock unsynced. TLS certificate validation may fail."));
  }

  HTTPClient http;
  if (!http.begin(_secureClient, url)) {
    result.errorMessage = "Failed to initialize HTTPS connection";
    return result;
  }

  http.setTimeout(15000);
  http.setFollowRedirects(HTTPC_DISABLE_REDIRECTS);

  const char* headerKeys[] = {"Location", "Content-Type"};
  http.collectHeaders(headerKeys, 2);
  http.addHeader("Content-Type", "application/json");

  int httpCode = http.POST(payload);
  result.httpCode = httpCode;

  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK) {
      result.success = true;
      result.responseBody = http.getString();
      logSanitized("[DeviceTelemetry] Direct response: ", result.responseBody);
    } else if (httpCode == HTTP_CODE_FOUND || httpCode == HTTP_CODE_SEE_OTHER || httpCode == 307 || httpCode == 308) {
      String redirectUrl = http.header("Location");
      http.end();

      if (redirectUrl.length() == 0 || !isSafeGoogleRedirect(redirectUrl)) {
        result.errorMessage = "Security violation: Invalid or empty redirect location";
        return result;
      }

      Serial.println(F("[DeviceTelemetry] Intercepted 302 redirect. Fetching receipt via GET..."));
      return executeGet(redirectUrl);
    } else {
      result.responseBody = http.getString();
      result.errorMessage = "HTTP Error " + String(httpCode);
      logSanitized("[DeviceTelemetry] Error response: ", result.responseBody);
    }
  } else {
    result.errorMessage = http.errorToString(httpCode);
    Serial.printf("[DeviceTelemetry] Connection failure (%d): %s\n", httpCode, result.errorMessage.c_str());
  }

  http.end();
  return result;
}

TelemetryResult DeviceTelemetry::executeGet(const String& url) {
  TelemetryResult result;
  result.success = false;
  result.httpCode = 0;

  HTTPClient http;
  if (!http.begin(_secureClient, url)) {
    result.errorMessage = "Failed to initialize HTTPS GET";
    return result;
  }

  http.setTimeout(15000);
  int httpCode = http.GET();
  result.httpCode = httpCode;

  if (httpCode == HTTP_CODE_OK) {
    result.success = true;
    result.responseBody = http.getString();
    logSanitized("[DeviceTelemetry] Receipt: ", result.responseBody);
  } else {
    result.responseBody = http.getString();
    result.errorMessage = "GET Error " + String(httpCode);
  }

  http.end();
  return result;
}

void DeviceTelemetry::logSanitized(const char* prefix, const String& content) {
  String sanitized = content;
  int pos = sanitized.indexOf("\"token\":");
  if (pos != -1) {
    int end = sanitized.indexOf(',', pos);
    if (end == -1) end = sanitized.indexOf('}', pos);
    if (end != -1) {
      sanitized = sanitized.substring(0, pos) + "\"token\":\"[REDACTED]\"" + sanitized.substring(end);
    }
  }
  Serial.print(prefix);
  Serial.println(sanitized);
}
