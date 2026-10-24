# Verification & Test Plan — IoT Sentinel

This document details the test procedures, expected inputs, observed responses, and pass/fail criteria for verifying the **IoT Sentinel** firmware and serverless receiver.

> [!IMPORTANT]
> A successful HTTP 200 response on the `doGet` health endpoint only proves that Google's web front-end can dispatch requests to Apps Script. It does **not** prove that Google Drive storage permissions, Script Properties, or JSON file creation logic are functioning. File creation must be independently verified via `doPost` telemetry tests.

---

## Test Execution Matrix

| Test ID | Category | Target Component | Execution Environment |
| :--- | :--- | :--- | :--- |
| **TC-01** | Cloud Receiver | Health Check Endpoint (`doGet`) | CLI (`curl`) / Web Browser |
| **TC-02** | Cloud Receiver | Valid Telemetry Event Ingestion | CLI (`curl`) / Drive UI |
| **TC-03** | Security | Invalid Authentication Token | CLI (`curl`) |
| **TC-04** | Robustness | Malformed JSON Payload | CLI (`curl`) |
| **TC-05** | Schema | Unsupported Event Type | CLI (`curl`) |
| **TC-06** | DoS Protection | Oversized Request Payload (>32 KB) | CLI (`curl`) / Python script |
| **TC-07** | Firmware | ESP32 Initial Boot & Startup Event | Physical ESP32 + Serial Monitor |
| **TC-08** | Firmware | 60-Second Periodic Heartbeat | Physical ESP32 + Serial Monitor |
| **TC-09** | Resilience | Wi-Fi Disconnect & Recovery | Physical ESP32 + Access Point |
| **TC-10** | Hardware State| Interactive LED State Change (`'l'`) | Physical ESP32 + Serial Monitor |
| **TC-11** | Local Logic | Telemetry Serializer & Schema Mock | Local Host (Automated Test Runner) |

---

## Detailed Test Procedures

### TC-01: Health Endpoint Check (`doGet`)
* **Objective**: Verify that the Google Apps Script Web App is deployed and reachable over public HTTPS.
* **Command**:
  ```bash
  curl -L -s "https://script.google.com/macros/s/YOUR_DEPLOYMENT_ID/exec"
  ```
* **Expected Output**:
  ```json
  {
    "status": "healthy",
    "service": "IoT Sentinel Telemetry Receiver",
    "version": "1.0.0",
    "timestamp_utc": "..."
  }
  ```
* **Pass Criteria**: Status code 200; JSON contains `"status": "healthy"`.

---

### TC-02: Valid Telemetry Event Ingestion
* **Objective**: Confirm that a conforming JSON payload with a valid token creates a timestamped file in Google Drive.
* **Command**:
  ```bash
  curl -L -s -X POST \
    "https://script.google.com/macros/s/YOUR_DEPLOYMENT_ID/exec" \
    -H "Content-Type: application/json" \
    -d '{
      "device_id": "esp32-sentinel-01",
      "event": "test",
      "firmware_version": "1.0.0",
      "uptime_seconds": 30,
      "wifi_connected": true,
      "wifi_rssi": -55,
      "led_state": "ON",
      "token": "YOUR_CONFIGURED_DEVICE_TOKEN"
    }'
  ```
* **Expected Output**:
  ```json
  {
    "status": "success",
    "file_id": "1xyz...",
    "filename": "telemetry_esp32-sentinel-01_test_...json",
    "received_at": "..."
  }
  ```
* **Pass Criteria**:
  1. Success response returned.
  2. New file visible in Google Drive folder.
  3. The saved file contains all telemetry metrics and a server receipt timestamp.
  4. **The `token` field is completely stripped from the saved file.**

---

### TC-03: Invalid Authentication Token
* **Objective**: Verify that requests with incorrect credentials are unequivocally rejected.
* **Command**:
  ```bash
  curl -L -s -X POST \
    "https://script.google.com/macros/s/YOUR_DEPLOYMENT_ID/exec" \
    -H "Content-Type: application/json" \
    -d '{
      "device_id": "esp32-sentinel-01",
      "event": "test",
      "firmware_version": "1.0.0",
      "uptime_seconds": 30,
      "wifi_connected": true,
      "wifi_rssi": -55,
      "led_state": "ON",
      "token": "FORGED_OR_INVALID_TOKEN"
    }'
  ```
* **Expected Output**:
  ```json
  {
    "status": "error",
    "code": "UNAUTHORIZED",
    "message": "Invalid or missing authentication token."
  }
  ```
* **Pass Criteria**: HTTP response contains error code `UNAUTHORIZED`. **No file is created in Drive.**

---

### TC-04: Malformed JSON Payload
* **Objective**: Ensure syntax errors or truncated payloads do not cause unhandled crashes.
* **Command**:
  ```bash
  curl -L -s -X POST \
    "https://script.google.com/macros/s/YOUR_DEPLOYMENT_ID/exec" \
    -H "Content-Type: application/json" \
    -d '{"device_id": "esp32-sentinel-01", "event": "test", BROKEN_JSON...'
  ```
* **Expected Output**:
  ```json
  {
    "status": "error",
    "code": "MALFORMED_JSON",
    "message": "Request body is not valid JSON."
  }
  ```
* **Pass Criteria**: Safe error response; no stack traces exposed.

---

### TC-05: Unsupported Event Type
* **Objective**: Verify that unknown or malicious event names are filtered out by the allowlist.
* **Command**:
  ```bash
  curl -L -s -X POST \
    "https://script.google.com/macros/s/YOUR_DEPLOYMENT_ID/exec" \
    -H "Content-Type: application/json" \
    -d '{
      "device_id": "esp32-sentinel-01",
      "event": "unauthorized_exploit_event",
      "firmware_version": "1.0.0",
      "uptime_seconds": 30,
      "wifi_connected": true,
      "wifi_rssi": -55,
      "led_state": "ON",
      "token": "YOUR_CONFIGURED_DEVICE_TOKEN"
    }'
  ```
* **Expected Output**:
  ```json
  {
    "status": "error",
    "code": "INVALID_EVENT",
    "message": "Field \"event\" must be one of: startup, test, heartbeat, led_change, alert"
  }
  ```
* **Pass Criteria**: Rejection with `INVALID_EVENT`.

---

### TC-06: Oversized Request Payload
* **Objective**: Confirm that payloads exceeding 32 KB are rejected to prevent cloud resource abuse.
* **Procedure**: Transmit a JSON string with padding exceeding 32,768 bytes.
* **Expected Output**:
  ```json
  {
    "status": "error",
    "code": "PAYLOAD_TOO_LARGE",
    "message": "Request payload exceeds maximum permitted size (32 KB)."
  }
  ```
* **Pass Criteria**: Request rejected without attempting to parse the full body.

---

### TC-07: ESP32 Hardware Startup Event
* **Objective**: Verify that the physical ESP32 connects to Wi-Fi, syncs NTP time, and sends a `"startup"` event.
* **Procedure**:
  1. Flash firmware with valid credentials in `secrets.h`.
  2. Open Serial Monitor at 115200 baud.
* **Observed Output**:
  - `[WiFiManager] Connected! IP Address: ...`
  - `[WiFiManager] Time synchronization successful!`
  - `[IoTSentinel] Transmitting initial 'startup' event...`
  - `[GoogleDriveClient] Server responded with HTTP status: 302`
  - `[IoTSentinel] SUCCESS: Event 'startup' safely logged to Google Drive!`
* **Pass Criteria**: Initial startup file present in Google Drive.

---

### TC-08: 60-Second Periodic Heartbeat
* **Objective**: Ensure the non-blocking timer triggers periodic updates without drift or blocking.
* **Procedure**: Leave the ESP32 powered for 3 minutes.
* **Pass Criteria**: Exactly three new heartbeat files created in Drive at ~60-second intervals.

---

### TC-09: Wi-Fi Disconnect and Recovery
* **Objective**: Verify that network interruptions do not freeze the microcontroller.
* **Procedure**:
  1. While ESP32 is running, disable the Wi-Fi Access Point or router.
  2. Observe Serial Monitor:
     - `[WiFiManager] WARNING: Wi-Fi connection lost. Entering non-blocking reconnect loop.`
     - Device continues servicing loop tasks without hanging.
  3. Re-enable the Wi-Fi Access Point.
  4. Observe Serial Monitor:
     - `[WiFiManager] Wi-Fi restored! IP: ...`
     - Normal telemetry transmission resumes automatically.
* **Pass Criteria**: ESP32 recovers network link automatically without requiring a physical reset.

---

### TC-10: Interactive Hardware LED Toggle
* **Objective**: Verify that changing hardware state triggers an immediate event.
* **Procedure**:
  1. In Arduino IDE Serial Monitor, type `l` and press **Enter**.
* **Observed Output**:
  - Physical LED toggles state.
  - `[IoTSentinel] Triggering 'led_change' telemetry event...`
  - Event confirmed and logged in Drive.
* **Pass Criteria**: Drive folder receives `telemetry_esp32-sentinel-01_led_change_...json` reflecting the new LED state.
