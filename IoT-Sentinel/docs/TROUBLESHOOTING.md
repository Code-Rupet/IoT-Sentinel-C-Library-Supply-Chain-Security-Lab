# Troubleshooting Guide — IoT Sentinel

This guide provides diagnostic procedures and verified solutions for common network, TLS, Google Apps Script, and hardware errors encountered when deploying IoT Sentinel.

---

## 1. Wi-Fi Connectivity Issues

### Symptom: `Initial Wi-Fi connection timed out. Background reconnection active.`
* **Cause 1: 5 GHz Wi-Fi Network**: ESP32 hardware radios only support the 2.4 GHz ISM band (802.11 b/g/n). If your router uses a unified SSID for both 2.4 GHz and 5 GHz, the ESP32 may fail to associate.
  * **Solution**: Split your Wi-Fi router into separate 2.4 GHz and 5 GHz SSIDs, or connect the ESP32 to a 2.4 GHz guest network or mobile hotspot.
* **Cause 2: Captive Portal / WPA2-Enterprise**: University and hotel Wi-Fi networks usually require a browser login (Captive Portal) or 802.1x username/password authentication (`WPA2-Enterprise`), which standard `WiFi.begin(ssid, password)` does not support.
  * **Solution**: Connect the ESP32 to a standard WPA2-Personal network or a mobile phone Wi-Fi hotspot.
* **Cause 3: Typo in `secrets.h`**:
  * **Solution**: Double-check `SECRET_WIFI_SSID` and `SECRET_WIFI_PASSWORD` in `secrets.h`. Strings are case-sensitive.

---

## 2. TLS & Clock Synchronization Issues

### Symptom: `[TLS WARNING] Device time is not yet synced via NTP. Certificate verification might fail!`
* **Explanation**: Microcontrollers have no battery-backed clock. When powered on, their clock starts at Jan 1, 1970. Since Google's TLS certificate was issued in 2016 and is valid until 2036, mbedTLS compares 1970 against 2016 and rejects the certificate as not yet valid (`MBEDTLS_X509_BADCERT_FUTURE`).
* **Solution**: Ensure your network allows outbound UDP traffic on **Port 123 (NTP)**. If your university campus firewall blocks NTP, specify an internal campus NTP server in `config.h` (e.g., `#define NTP_SERVER_PRIMARY "ntp.university.edu"`).

### Symptom: `TLS/Network Error (-1): connection failed` or `mbedtls -0x2700`
* **Explanation**: The TLS handshake was aborted during certificate path validation.
* **Solution**:
  1. Confirm that `GOOGLE_GTS_ROOT_CA` in `config.h` matches the GTS Root R1 certificate exactly.
  2. Verify that NTP synchronized properly before the transmission attempt.
  3. Ensure your Google Apps Script URL uses `https://`, not `http://`.

---

## 3. Google Apps Script HTTP Response Codes

### HTTP 401: `Invalid or missing authentication token.`
* **Explanation**: The token transmitted in `SECRET_DEVICE_TOKEN` does not match the `DEVICE_TOKEN` property stored in Google Apps Script.
* **Solution**:
  1. Open your Apps Script project at [script.google.com](https://script.google.com/).
  2. Click **Project Settings** (⚙️) > **Script Properties**.
  3. Verify that `DEVICE_TOKEN` is present and matches `SECRET_DEVICE_TOKEN` in `firmware/IoTSentinel/secrets.h` character-for-character.
  4. Ensure you created a **New Version** deployment after editing script properties.

### HTTP 500: `Receiver configuration is incomplete.`
* **Explanation**: Either `DEVICE_TOKEN` or `DRIVE_FOLDER_ID` is missing from Script Properties.
* **Solution**: Add both keys in **Project Settings > Script Properties** and redeploy.

### HTTP 400: `Field "event" must be one of: startup, test, heartbeat, led_change, alert`
* **Explanation**: The firmware transmitted an event name not present in the receiver's allowlist.
* **Solution**: Check `ALLOWED_EVENTS` in `Code.gs` or use one of the standard supported events.

### HTTP 405: `Method Not Allowed` (on Redirection)
* **Explanation**: This occurs if you attempt to use third-party HTTP libraries with blind redirect following that repeats a `POST` request to `script.googleusercontent.com`.
* **Solution**: The `GoogleDriveClient.cpp` in IoT Sentinel already prevents this by intercepting the 302/303 redirect and issuing a clean `GET` request to the content server. Ensure you use the provided `GoogleDriveClient` implementation.

---

## 4. Google Drive Storage Issues

### Symptom: Apps Script returns success, but no files appear in Google Drive
* **Cause 1: Stale Deployment Version**: In Google Apps Script, editing code in the editor does **not** update the public web app automatically.
  * **Solution**: Click **Deploy** > **Manage deployments** > Click the edit pencil icon ✏️ > Under *Version*, select **New version** > Click **Deploy**.
* **Cause 2: Incorrect Folder ID**: If `DRIVE_FOLDER_ID` contains trailing spaces or invalid characters, files may be created in your root Drive or discarded.
  * **Solution**: Open the folder in your browser, copy only the alphanumeric string between `/folders/` and any `?` query parameters, and update the Script Property.

---

## 5. Hardware & Arduino IDE Issues

### Symptom: `Failed to connect to ESP32: Timed out waiting for packet header`
* **Explanation**: The ESP32 bootloader was not triggered during serial programming.
* **Solution**:
  1. When Arduino IDE displays `Connecting........_____.....`, press and hold the physical **BOOT** (or **IO0**) button on the ESP32 board.
  2. Release the button as soon as you see the flashing progress percentage (`Writing at 0x00010000... (5 %)`).
  3. Alternatively, solder a 10µF electrolytic capacitor between the `EN` pin and `GND` to automate bootloader entry.

### Symptom: Diagnostic LED does not turn ON
* **Explanation**: Not all ESP32 development boards connect the onboard LED to GPIO 2.
  * ESP32-C3 boards typically use **GPIO 8**.
  * ESP32-S3 boards typically use **GPIO 21** or **GPIO 48**.
  * Some boards (e.g. Wemos LOLIN32) use an active-low LED configuration.
* **Solution**:
  1. Open `firmware/IoTSentinel/config.h`.
  2. Adjust `STATUS_LED_PIN` to match your board's pinout.
  3. If your LED turns off when it should be on, change `#define STATUS_LED_ACTIVE_HIGH false`.

### Symptom: Compilation error `DynamicJsonDocument does not name a type`
* **Explanation**: ArduinoJson v7 changed its memory API from `StaticJsonDocument`/`DynamicJsonDocument` to a unified `JsonDocument`.
* **Solution**: IoT Sentinel includes automatic version detection macros in `Telemetry.cpp`. Verify that your `Telemetry.cpp` file contains `#if ARDUINOJSON_VERSION_MAJOR >= 7`.
