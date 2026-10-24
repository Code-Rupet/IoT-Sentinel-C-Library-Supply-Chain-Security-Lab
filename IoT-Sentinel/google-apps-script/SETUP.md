# Google Apps Script Telemetry Receiver Setup

This guide details how to configure and deploy the serverless Google Apps Script receiver that writes ESP32 telemetry events to a dedicated Google Drive folder.

---

## 1. Create the Google Drive Storage Folder

1. Open [Google Drive](https://drive.google.com/).
2. Click **+ New** > **New folder**.
3. Name the folder: `IoT-Sentinel-Telemetry`.
4. Open the newly created folder.
5. Inspect the URL in your browser's address bar:
   ```text
   https://drive.google.com/drive/folders/1a2B3c4D5e6F7g8H9i0J_kLmNoPqRsTuV
   ```
6. Copy the string of characters after `/folders/` (e.g., `1a2B3c4D5e6F7g8H9i0J_kLmNoPqRsTuV`). This is your `DRIVE_FOLDER_ID`. Keep this value handy.

---

## 2. Create the Google Apps Script Project

1. Navigate to [script.google.com](https://script.google.com/) or simply open [script.new](https://script.new) in your browser.
2. Rename the project in the upper left from *Untitled project* to `IoT-Sentinel-Receiver`.
3. Open the file `Code.gs` in the editor.
4. Replace all existing boilerplate code with the entire contents of [`Code.gs`](file:///home/sherlock/.gemini/antigravity-ide/scratch/IoT-Sentinel/google-apps-script/Code.gs).

### (Optional but Recommended) Enable Manifest View
1. Click the **Project Settings** gear icon (⚙️) on the left sidebar.
2. Check the box **Show "appsscript.json" manifest file in editor**.
3. Click the **Editor** (`< >`) icon on the left sidebar.
4. Click on `appsscript.json` and replace its content with [`appsscript.json`](file:///home/sherlock/.gemini/antigravity-ide/scratch/IoT-Sentinel/google-apps-script/appsscript.json).

---

## 3. Configure Script Properties (Environment Secrets)

Never hardcode folder IDs or authentication tokens directly in `Code.gs`. Use Google Apps Script's encrypted Script Properties store:

1. Click the **Project Settings** gear icon (⚙️) on the left sidebar.
2. Scroll down to the **Script Properties** section.
3. Click **Add script property** and add the following two mandatory rows (and one optional):

| Property Name | Example Value | Description |
| :--- | :--- | :--- |
| `DEVICE_TOKEN` | `sentinel_sec_9876543210abcdef` | Generate a strong random token. Must match `DEVICE_AUTH_TOKEN` in ESP32 `secrets.h`. |
| `DRIVE_FOLDER_ID` | `1a2B3c4D5e6F7g8H9i...` | The Google Drive folder ID copied in Section 1. |
| `EXPECTED_DEVICE_ID` | `esp32-sentinel-01` | *(Optional)* Restricts uploads to only this specific device identifier. |

4. Click **Save script properties**.

---

## 4. Deploy as a Web App

1. In the upper right corner, click **Deploy** > **New deployment**.
2. Click the gear icon next to *Select type* and select **Web app**.
3. Configure deployment settings:
   - **Description**: `IoT Sentinel Telemetry Receiver v1`
   - **Execute as**: **Me (`your-email@gmail.com`)**  
     *(Ensures script executes with your permissions to write to your Drive folder)*
   - **Who has access**: **Anyone**  
     *(Required so the headless ESP32 micro-controller can transmit HTTPS telemetry without an interactive OAuth browser login)*
4. Click **Deploy**.
5. **Authorize Access**:
   - Google will prompt you to review permissions.
   - Click **Authorize access**.
   - Select your Google account.
   - If you see *Google hasn't verified this app*, click **Advanced** (bottom left), then click **Go to IoT-Sentinel-Receiver (unsafe)**.
   - Review permissions (access Google Drive files) and click **Allow**.
6. **Copy the Web App URL**:
   The deployment dialog will present a URL formatted as:
   ```text
   https://script.google.com/macros/s/AKfycbx...ABC123xyz/exec
   ```
   **Copy this URL.** You will paste it into `firmware/IoTSentinel/secrets.h` as `GOOGLE_SCRIPT_URL`.

> [!WARNING]
> Whenever you modify code in `Code.gs`, you MUST create a **New Version** via **Deploy > Manage deployments > Edit > New version > Deploy**. Updating the code without creating a new version deployment will continue executing the previous code version.

---

## 5. Verify the Deployment from the Command Line

Before flashing the ESP32, verify both the health endpoint and the telemetry submission from your computer terminal.

### Test 1: Verify Health Endpoint (`doGet`)
```bash
curl -L -s "https://script.google.com/macros/s/YOUR_DEPLOYMENT_ID/exec"
```
**Expected Output:**
```json
{
  "status": "healthy",
  "service": "IoT Sentinel Telemetry Receiver",
  "version": "1.0.0",
  "timestamp_utc": "2026-10-24T21:25:00.000Z"
}
```

### Test 2: Verify Telemetry Ingestion (`doPost`)
```bash
curl -L -s -X POST \
  "https://script.google.com/macros/s/YOUR_DEPLOYMENT_ID/exec" \
  -H "Content-Type: application/json" \
  -d '{
    "device_id": "esp32-sentinel-01",
    "event": "test",
    "firmware_version": "1.0.0",
    "uptime_seconds": 12,
    "wifi_connected": true,
    "wifi_rssi": -55,
    "led_state": "ON",
    "token": "YOUR_CONFIGURED_DEVICE_TOKEN"
  }'
```
**Expected Output:**
```json
{
  "status": "success",
  "file_id": "1xyz...",
  "filename": "telemetry_esp32-sentinel-01_test_2026-10-24T21-25-00-000Z_a1b2c3d4.json",
  "received_at": "2026-10-24T21:25:00.000Z"
}
```

### Test 3: Check Google Drive
Open your `IoT-Sentinel-Telemetry` folder in Google Drive. You will see the new `.json` file. Double-click it to verify:
- The telemetry fields are intact.
- Server timestamp `server_received_at_utc` is appended.
- **The authentication `token` has been stripped and is NOT stored in Google Drive.**
