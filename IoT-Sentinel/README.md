# IoT Sentinel — C++ Library Supply-Chain Security Lab

[![Firmware Language](https://img.shields.io/badge/Language-C%2B%2B%20%28Arduino%29-blue.svg)](https://www.arduino.cc/)
[![Target Hardware](https://img.shields.io/badge/Hardware-ESP32%20DevKit-green.svg)](https://www.espressif.com/)
[![Cloud Storage](https://img.shields.io/badge/Storage-Google%20Drive-yellow.svg)](https://drive.google.com/)
[![Security Research](https://img.shields.io/badge/Topic-Supply--Chain%20Security-red.svg)](https://csrc.nist.gov/publications/detail/sp/800-161/rev-1/final)

**IoT Sentinel** is an empirical, university-grade research testbed designed to investigate how third-party C++ libraries introduce unexpected runtime behavior and network communications into embedded IoT applications, and how developers can detect and analyze that behavior.

---

## 1. Research Question

> **Can a third-party C++ library introduce network communication or runtime behavior that differs from what a developer expects based on the main application's source code?**

Because standard microcontroller runtimes (like the ESP32 under Arduino/FreeRTOS) execute in a **flat address space without memory protection or process isolation**, any imported C++ library runs with supervisor privileges. This testbed allows researchers and students to investigate this supply-chain risk through controlled, repeatable laboratory experiments.

---

## 2. Controlled Laboratory Configurations

The testbed features three distinct compile-time configurations in [`firmware/IoTSentinel/config.h`](file:///home/sherlock/.gemini/antigravity-ide/scratch/IoT-Sentinel/firmware/IoTSentinel/config.h):

| Configuration | Architectural Setup | Network Behavior | Research Purpose |
| :--- | :--- | :--- | :--- |
| **`CONFIG_A` — Baseline** | `SmartLED` active. Zero telemetry libraries instantiated. | Local Wi-Fi association; DHCP & NTP only. **Zero HTTPS packets.** | Establishes behavioral and network baseline for the pure actuator application. |
| **`CONFIG_B` — Approved Telemetry** | `SmartLED` + documented `DeviceTelemetry` library. | Periodic 60s heartbeats and actuator state updates to Google Apps Script. | Records legitimate, expected telemetry profile and TLS 1.3 handshake patterns. |
| **`CONFIG_C` — Anomaly Simulation** | `SmartLED` + `DeviceTelemetry` emitting synthetic anomalies. | Dual-burst TLS transmissions upon actuator trigger; unapproved metadata. | Demonstrates how unexpected library side-effects trigger cloud-side and network alerts. |

---

## 3. Repository Architecture

```text
IoT-Sentinel/
├── README.md                          # Master overview and quickstart (this file)
├── .gitignore                         # Excludes secrets.h and build artifacts
├── firmware/
│   └── IoTSentinel/
│       ├── IoTSentinel.ino            # Main research sketch & interactive Serial CLI
│       ├── config.h                   # Experiment configuration selectors & GTS Root CA
│       └── secrets.example.h          # Credentials template (copy to secrets.h)
├── libraries/
│   ├── SmartLED/                      # Pure hardware actuator library
│   │   ├── library.properties         # Arduino library metadata
│   │   └── src/
│   │       ├── SmartLED.h             # Actuator interface (begin, turnOn, turnOff, toggle)
│   │       └── SmartLED.cpp           # Direct GPIO pin driver
│   └── DeviceTelemetry/               # Application-level telemetry library
│       ├── library.properties         # Arduino library metadata
│       └── src/
│           ├── DeviceTelemetry.h      # Telemetry interface & privacy disclosures
│           └── DeviceTelemetry.cpp    # mbedTLS client with Google redirect & anomaly simulation
├── google-apps-script/
│   ├── Code.gs                        # Cloud receiver with supply-chain anomaly detection
│   └── SETUP.md                       # Google Drive & Apps Script deployment guide
├── tests/
│   ├── test-plan.md                   # Step-by-step laboratory testing workflows
│   ├── expected-behavior.json         # Approved baseline telemetry specification
│   ├── simulated-anomaly.json         # Synthetic anomaly specification (Config C)
│   ├── test_apps_script.js            # Automated Node.js unit tests for Code.gs
│   └── test_redirect_validator.cpp    # Native C++ unit tests for redirect validation
└── docs/
    ├── ARCHITECTURE.md                # System topology, sequence flow, and memory models
    ├── THREAT-MODEL.md                # STRIDE & NIST SP 800-161 embedded supply-chain analysis
    ├── DEPENDENCY-ANALYSIS.md         # Source review checklist & SHA-256 integrity baseline
    ├── NETWORK-ANALYSIS.md            # Wireshark inspection procedures & comparative table
    └── FINAL-REPORT.md                # Comprehensive academic research report
```

---

## 4. Quickstart Guide

### Step 4.1: Deploy the Cloud Ingestion Receiver
1. Follow the instructions in [`google-apps-script/SETUP.md`](file:///home/sherlock/.gemini/antigravity-ide/scratch/IoT-Sentinel/google-apps-script/SETUP.md) to create an `IoT-Sentinel-Telemetry` folder in Google Drive.
2. Deploy [`google-apps-script/Code.gs`](file:///home/sherlock/.gemini/antigravity-ide/scratch/IoT-Sentinel/google-apps-script/Code.gs) as a Web App with access set to **Anyone**.
3. Configure `DEVICE_TOKEN` and `DRIVE_FOLDER_ID` in Script Properties.
4. Copy your **Web App URL**.

### Step 4.2: Configure Firmware Secrets
1. Copy the secrets template:
   ```bash
   cp firmware/IoTSentinel/secrets.example.h firmware/IoTSentinel/secrets.h
   ```
2. Open `secrets.h` and populate your Wi-Fi SSID, password, Web App URL, and Device Token.

### Step 4.3: Select Experiment Configuration
Open [`firmware/IoTSentinel/config.h`](file:///home/sherlock/.gemini/antigravity-ide/scratch/IoT-Sentinel/firmware/IoTSentinel/config.h) and set:
```cpp
#define EXPERIMENT_CONFIG CONFIG_B // or CONFIG_A or CONFIG_C
```

### Step 4.4: Flash and Monitor
1. Open [`firmware/IoTSentinel/IoTSentinel.ino`](file:///home/sherlock/.gemini/antigravity-ide/scratch/IoT-Sentinel/firmware/IoTSentinel/IoTSentinel.ino) in Arduino IDE.
2. Select your ESP32 board and COM port.
3. Click **Upload** and open the **Serial Monitor** at **115200 baud**.
4. Test interactive commands:
   - `l` : Toggle SmartLED (generates network event in Config B & C).
   - `t` : Send manual test event.
   - `h` : Send manual heartbeat.
   - `c` : Manually trigger synthetic anomaly (Config C).
   - `s` : Display live hardware diagnostic status.

---

## 5. Running Automated Local Tests

To verify the cloud receiver logic and redirect filtering algorithms without hardware:

```bash
# 1. Run Google Apps Script unit tests (Node.js)
node tests/test_apps_script.js

# 2. Compile and run C++ Google redirect domain validator (g++)
g++ -Wall -Wextra tests/test_redirect_validator.cpp -o tests/test_redirect_validator
./tests/test_redirect_validator
```

---

## 6. Research Documentation Directory

* [**Final Academic Research Report**](file:///home/sherlock/.gemini/antigravity-ide/scratch/IoT-Sentinel/docs/FINAL-REPORT.md)
* [**Threat Model & STRIDE Analysis**](file:///home/sherlock/.gemini/antigravity-ide/scratch/IoT-Sentinel/docs/THREAT-MODEL.md)
* [**Dependency Inventory & SHA-256 Audit**](file:///home/sherlock/.gemini/antigravity-ide/scratch/IoT-Sentinel/docs/DEPENDENCY-ANALYSIS.md)
* [**Wireshark Network Traffic Analysis**](file:///home/sherlock/.gemini/antigravity-ide/scratch/IoT-Sentinel/docs/NETWORK-ANALYSIS.md)
* [**System Architecture & Sequence Flows**](file:///home/sherlock/.gemini/antigravity-ide/scratch/IoT-Sentinel/docs/ARCHITECTURE.md)
* [**Laboratory Test Plan**](file:///home/sherlock/.gemini/antigravity-ide/scratch/IoT-Sentinel/tests/test-plan.md)

---

## License

This project is licensed under the Apache 2.0 Open Source License for academic, educational, and research purposes.
