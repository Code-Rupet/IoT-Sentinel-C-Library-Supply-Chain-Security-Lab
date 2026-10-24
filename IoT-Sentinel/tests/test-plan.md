# Experimental Test Plan — IoT Sentinel Supply-Chain Lab

This document defines the laboratory testing procedures for investigating how third-party C++ libraries introduce unexpected runtime behavior and how embedded developers can detect that behavior.

---

## 1. Test Environment Matrix

| Test Suite | Execution Target | Requirements | Verification Point |
| :--- | :--- | :--- | :--- |
| **Suite 1: Local Automated Tests** | Local Host (Node.js & C++) | Terminal / Compilers | Code.gs logic, redirect validator, payload schemas |
| **Suite 2: Configuration A (Baseline)** | Physical ESP32 | ESP32 board, Wi-Fi | Zero telemetry; SmartLED operates locally |
| **Suite 3: Configuration B (Approved Telemetry)** | Physical ESP32 + Cloud | ESP32 + Google Drive | Approved telemetry ingested on 60s schedule |
| **Suite 4: Configuration C (Anomaly Simulation)** | Physical ESP32 + Cloud | ESP32 + Google Drive | Synthetic anomaly detected and tagged with `ALERT_` |
| **Suite 5: Network Traffic Analysis** | Workstation (Wireshark) | Wi-Fi Packet Monitor | Protocol breakdown & destination validation |
| **Suite 6: Dependency Integrity Audit** | Host Terminal | `sha256sum` | Cryptographic hash comparison against baseline |

---

## 2. Laboratory Experiment Workflows

### Experiment 1: Configuration A — Baseline (No Telemetry)
* **Goal**: Establish the network and behavioral baseline of the IoT device without telemetry libraries.
* **Firmware Setting**: In [`firmware/IoTSentinel/config.h`](file:///home/sherlock/.gemini/antigravity-ide/scratch/IoT-Sentinel/firmware/IoTSentinel/config.h), set:
  ```cpp
  #define EXPERIMENT_CONFIG CONFIG_A
  ```
* **Procedure**:
  1. Flash the ESP32 and open Serial Monitor at 115200 baud.
  2. Start Wireshark on your computer monitoring the ESP32 IP address (`ip.addr == <ESP32_IP>`).
  3. Type `l` into the Serial Monitor to toggle the LED.
* **Expected Observations**:
  - LED toggles physical state ON and OFF.
  - Serial Monitor outputs: `[CONFIG_A] LED toggled. Zero network traffic generated.`
  - In Wireshark, **zero TCP port 443 packets** are generated during LED toggles. Only DHCP and NTP maintenance packets appear.

---

### Experiment 2: Configuration B — Approved Telemetry
* **Goal**: Observe the legitimate, documented telemetry profile of the `DeviceTelemetry` library.
* **Firmware Setting**: In [`firmware/IoTSentinel/config.h`](file:///home/sherlock/.gemini/antigravity-ide/scratch/IoT-Sentinel/firmware/IoTSentinel/config.h), set:
  ```cpp
  #define EXPERIMENT_CONFIG CONFIG_B
  ```
* **Procedure**:
  1. Flash the ESP32 and open Serial Monitor.
  2. Observe startup transmission and 60-second heartbeat transmissions.
  3. Type `l` to toggle the LED and observe immediate event dispatch.
* **Expected Observations**:
  - Serial Monitor confirms: `Telemetry event 'startup' verified by remote receiver.`
  - In Google Drive, standard files appear: `telemetry_esp32-sentinel-01_heartbeat_...json`.
  - In Wireshark, periodic TLS sessions connect strictly to `script.google.com` (TCP 443).
  - No unapproved domains or unusual ports appear in DNS lookups.

---

### Experiment 3: Configuration C — Simulated Compromised Dependency
* **Goal**: Simulate an unapproved third-party library introducing unexpected telemetry behavior and evaluate cloud detection.
* **Firmware Setting**: In [`firmware/IoTSentinel/config.h`](file:///home/sherlock/.gemini/antigravity-ide/scratch/IoT-Sentinel/firmware/IoTSentinel/config.h), set:
  ```cpp
  #define EXPERIMENT_CONFIG CONFIG_C
  ```
* **Procedure**:
  1. Flash the ESP32 and open Serial Monitor.
  2. Type `l` to toggle the LED.
  3. Notice that the third-party library automatically emits an additional synthetic event (`simulated_anomaly`).
* **Expected Observations**:
  - Serial Monitor warns: `[CONFIG_C:SIMULATION] Third-party dependency emitting unexpected synthetic telemetry...`
  - In Google Drive, the file is saved with the explicit alert prefix: `ALERT_telemetry_esp32-sentinel-01_simulated_anomaly_...json`.
  - Opening the file reveals:
    ```json
    {
      "event": "simulated_anomaly",
      "synthetic_anomaly_flag": true,
      "security_audit": {
        "anomaly_detected": true,
        "classification": "SUPPLY_CHAIN_RESEARCH_ANOMALY"
      }
    }
    ```
  - In Wireshark, the analyst observes **two consecutive TLS handshakes** for a single actuator event, deviating from the baseline single-packet model.

---

### Experiment 4: Dependency Hash Verification
* **Goal**: Audit library source files using cryptographic hashing to detect unauthorized code changes before compilation.
* **Procedure**:
  ```bash
  sha256sum libraries/SmartLED/src/* libraries/DeviceTelemetry/src/*
  ```
* **Expected Observations**:
  - Compare generated digests against the baseline inventory in [`docs/DEPENDENCY-ANALYSIS.md`](file:///home/sherlock/.gemini/antigravity-ide/scratch/IoT-Sentinel/docs/DEPENDENCY-ANALYSIS.md).
  - Any discrepancy indicates source modifications requiring immediate manual code review.
