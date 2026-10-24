# Setup Guide — IoT Sentinel

This step-by-step guide explains how to prepare your development computer, configure Google Drive and Apps Script, set up the Arduino IDE, configure credentials, and flash the ESP32.

---

## 1. Prerequisites

### Hardware
* **ESP32 Development Board** (e.g., ESP32-WROOM-32, NodeMCU-32S, ESP32 DevKit v1).
* **Data-capable USB cable** (Micro-USB or USB-C depending on your board; ensure it is not a power-only charging cable).
* **2.4 GHz Wi-Fi Network** (The ESP32 standard radio does not support 5 GHz Wi-Fi networks).

### Software
* [Arduino IDE 2.x](https://www.arduino.cc/en/software) (or 1.8.19+).
* A standard Google Account with access to [Google Drive](https://drive.google.com).
* CP210x or CH340 USB-to-UART bridge drivers (pre-installed on most modern Linux/macOS systems; Windows may require driver installation).

---

## 2. Google Cloud & Drive Setup

Follow the full instructions in [`google-apps-script/SETUP.md`](file:///home/sherlock/.gemini/antigravity-ide/scratch/IoT-Sentinel/google-apps-script/SETUP.md) to:
1. Create a Google Drive folder (e.g., `IoT-Sentinel-Telemetry`) and note its `DRIVE_FOLDER_ID`.
2. Create the Apps Script project at [script.new](https://script.new) using [`Code.gs`](file:///home/sherlock/.gemini/antigravity-ide/scratch/IoT-Sentinel/google-apps-script/Code.gs).
3. Set **Script Properties**:
   - `DEVICE_TOKEN`: A strong secret token (e.g. `sentinel_sec_9f83a2c4e1b7`).
   - `DRIVE_FOLDER_ID`: Your Drive folder ID.
4. Deploy the script as a **Web App**:
   - Execute as: **Me**
   - Access: **Anyone**
5. Copy the generated **Web App URL** (e.g., `https://script.google.com/macros/s/.../exec`).

---

## 3. Arduino IDE Environment Setup

### Step 3.1: Install the ESP32 Board Package
1. Open Arduino IDE.
2. Go to **File** > **Preferences** (on macOS: **Arduino IDE** > **Settings**).
3. In the **Additional boards manager URLs** field, paste:
   ```text
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
4. Click **OK**.
5. Open the Boards Manager (**Tools** > **Board** > **Boards Manager...** or click the board icon on the left sidebar).
6. Search for `esp32` by **Espressif Systems** and click **Install**.

### Step 3.2: Install Required Libraries
1. Open the Library Manager (**Tools** > **Manage Libraries...** or click the books icon on the left sidebar).
2. Search for `ArduinoJson` by **Benoit Blanchon**.
3. Select the latest version (v7.x or v6.x are both supported) and click **Install**.

*Note: `WiFi.h`, `WiFiClientSecure.h`, and `HTTPClient.h` are built directly into the Espressif ESP32 board package; no additional installation is needed.*

---

## 4. Firmware Configuration

### Step 4.1: Open the Project
1. In Arduino IDE, select **File** > **Open...**
2. Navigate to [`firmware/IoTSentinel/IoTSentinel.ino`](file:///home/sherlock/.gemini/antigravity-ide/scratch/IoT-Sentinel/firmware/IoTSentinel/IoTSentinel.ino) and open it. All companion files (`config.h`, `Telemetry.h`, `GoogleDriveClient.h`, etc.) will appear as tabs.

### Step 4.2: Create `secrets.h`
1. In the `firmware/IoTSentinel/` directory, create a copy of `secrets.example.h` named `secrets.h`:
   ```bash
   cp firmware/IoTSentinel/secrets.example.h firmware/IoTSentinel/secrets.h
   ```
2. Open `secrets.h` and fill in your actual values:
   ```cpp
   #define SECRET_WIFI_SSID         "Your_WiFi_Network_Name"
   #define SECRET_WIFI_PASSWORD     "Your_WiFi_Password"
   #define SECRET_GOOGLE_SCRIPT_URL "https://script.google.com/macros/s/AKfycb.../exec"
   #define SECRET_DEVICE_TOKEN      "sentinel_sec_9f83a2c4e1b7"
   ```
3. Save the file.

### Step 4.3: Check Board Pinout (`config.h`)
Open `config.h`. By default, the diagnostic status LED is set to **GPIO 2**:
```cpp
#define STATUS_LED_PIN 2
```
* **ESP32-WROOM-32 / DevKit v1 / NodeMCU-32S**: GPIO 2 is correct (blue onboard LED).
* **ESP32-C3 DevKit**: Change to `8`.
* **ESP32-S3 DevKit**: Change to `21` or `48`.
* **Boards with no onboard LED**: Connect an external 330Ω resistor and LED between GPIO 2 and GND.

---

## 5. Compiling, Flashing, and Monitoring

1. Connect your ESP32 board to your computer via USB.
2. Select your board under **Tools** > **Board** > **esp32** > **ESP32 Dev Module** (or your specific board model).
3. Select the serial port under **Tools** > **Port** (e.g., `/dev/ttyUSB0` on Linux, `/dev/cu.usbserial-...` on macOS, or `COM3` on Windows).
4. Click the **Upload** button (arrow icon ➔ in top left).
5. If the upload hangs at `Connecting........_____.....`:
   - Press and hold the physical **BOOT** (or **IO0**) button on your ESP32 board until the flashing process starts, then release it.
6. Once the message `Done uploading` appears, open the **Serial Monitor** (**Tools** > **Serial Monitor**).
7. Set the baud rate in the bottom right corner of the Serial Monitor to **115200 baud**.

---

## 6. Expected Serial Monitor Output

Upon booting, the ESP32 will output the following diagnostic logs:

```text
=======================================================
   IoT Sentinel — ESP32 to Google Drive Telemetry Node
   University IoT Cybersecurity Demonstration Firmware
   Device ID: esp32-sentinel-01 | Firmware: v1.0.0
=======================================================

[IoTSentinel] Hardware LED set to: ON (GPIO 2)
[WiFiManager] Connecting to Wi-Fi SSID: "Lab_Network" (Password: [PROTECTED])...
.....
[WiFiManager] Connected! IP Address: 192.168.1.145, RSSI: -54 dBm
[WiFiManager] Synchronizing clock via SNTP (pool.ntp.org, time.google.com)...
[WiFiManager] Time synchronization successful!
[WiFiManager] Current System Time: 2026-10-24 21:28:00 UTC

[IoTSentinel] Transmitting initial 'startup' event...
[IoTSentinel] Event [startup] prepared. Payload size: 218 bytes.
[GoogleDriveClient] Transmission attempt 1 of 3...
[GoogleDriveClient] Server responded with HTTP status: 302
[GoogleDriveClient] Following verified Google content redirect via HTTPS GET...
[GoogleDriveClient] Payload: {"status":"success","file_id":"1A2b3C...","filename":"telemetry_esp32-sentinel-01_startup_...json"}
[IoTSentinel] SUCCESS: Event 'startup' safely logged to Google Drive!

[IoTSentinel] Transmitting verification 'test' event...
[IoTSentinel] Event [test] prepared. Payload size: 215 bytes.
...
[IoTSentinel] SUCCESS: Event 'test' safely logged to Google Drive!

--- [INTERACTIVE LAB TEST COMMANDS] ---
 Type single character in Serial Monitor & press Enter:
   'l' -> Toggle LED & send 'led_change' event to Drive
   't' -> Trigger manual 'test' event to Drive
   'h' -> Trigger manual 'heartbeat' event to Drive
   's' -> Print real-time status and network diagnostic report
   '?' -> Show this help menu
---------------------------------------
```

---

## 7. Verify Drive Files

1. Open your Google Drive folder (`IoT-Sentinel-Telemetry`).
2. Refresh the browser.
3. You will see individual timestamped JSON files:
   - `telemetry_esp32-sentinel-01_startup_2026-10-24T..._a1b2c3d4.json`
   - `telemetry_esp32-sentinel-01_test_2026-10-24T..._e5f6g7h8.json`
4. Double-click any file to verify that the telemetry schema is intact and that no secret tokens are stored.
