# System Architecture — IoT Sentinel Supply-Chain Lab

This document details the architectural design, component modularity, sequence workflows, and trust models of the **IoT Sentinel** research testbed.

---

## 1. System Topology & Trust Domains

The research testbed is structured into three discrete execution tiers:

```mermaid
graph TD
    subgraph EdgeDevice ["Edge Tier (ESP32 Dev Board)"]
        App["Main Firmware (IoTSentinel.ino)"]
        SmartLEDLib["SmartLED Library (Actuator Control)"]
        TelemLib["DeviceTelemetry Library (Telemetry Engine)"]
        
        App -->|"Calls begin() / toggle()"| SmartLEDLib
        App -->|"Explicitly invokes sendEvent()"| TelemLib
        
        subgraph LibBoundary ["Library Execution Context"]
            SmartLEDLib -.->|"Direct GPIO access (Pin 2)"| Hardware["ESP32 Hardware Registers"]
            TelemLib -.->|"mbedTLS + GTS Root R1 CA"| NetworkStack["ESP32 TCP/IP & Wi-Fi Stack"]
        end
    end

    subgraph TransitTier ["Encrypted Transit Tier (Public WAN)"]
        NTP["NTP Pool (pool.ntp.org:123)"]
        NetworkStack -.->|"SNTP (UDP 123)"| NTP
        NetworkStack ==>|"HTTPS (TLS 1.2/1.3:443)"| GoogleFront["Google Front End (script.google.com)"]
    end

    subgraph CloudTier ["Serverless Cloud Tier (Google Workspace)"]
        GoogleFront -->|"Route doPost(e)"| AppsScript["Google Apps Script Engine (Code.gs)"]
        
        subgraph IngestionPipeline ["Analysis & Storage Pipeline"]
            TokenVal["Token & Schema Validator"]
            AnomalyDetector["Supply-Chain Anomaly Detector"]
            Sanitizer["Token Stripper & Filename Generator"]
            
            AppsScript --> TokenVal
            TokenVal --> AnomalyDetector
            AnomalyDetector --> Sanitizer
        end

        Sanitizer -->|"DriveApp.createFile()"| GDrive["Google Drive Storage Folder (.json)"]
    end
```

---

## 2. Experimental Configurations

To investigate the research question, the firmware supports three compile-time configurations in [`config.h`](file:///home/sherlock/.gemini/antigravity-ide/scratch/IoT-Sentinel/firmware/IoTSentinel/config.h):

```
+---------------------------------------------------------------------------------+
|                        RESEARCH EXPERIMENT CONFIGURATIONS                       |
+---------------------------------------------------------------------------------+
| Configuration A : Baseline (Zero Telemetry)                                     |
|                   - SmartLED active (GPIO 2 toggling)                           |
|                   - Wi-Fi associated and clock synced                           |
|                   - DeviceTelemetry library NOT instantiated                     |
|                   - Expected Network Traffic: DHCP + NTP only. Zero HTTPS.      |
+---------------------------------------------------------------------------------+
| Configuration B : Approved Telemetry                                            |
|                   - SmartLED active                                             |
|                   - DeviceTelemetry instantiated with explicit parameters       |
|                   - Approved Events: 'startup', 'heartbeat' (60s), 'led_change' |
|                   - Expected Network Traffic: TLS sessions strictly to Google   |
+---------------------------------------------------------------------------------+
| Configuration C : Simulated Compromised Dependency                              |
|                   - SmartLED active                                             |
|                   - DeviceTelemetry active                                      |
|                   - Injects synthetic unexpected event ('simulated_anomaly')    |
|                   - Cloud pipeline flags event with 'ALERT_' prefix in Drive    |
|                   - Wireshark captures multiple TLS handshakes per user action  |
+---------------------------------------------------------------------------------+
```

---

## 3. Telemetry Sequence & Google Redirection Flow

The following sequence diagram illustrates how telemetry is transmitted, validated, and stored:

```mermaid
sequenceDiagram
    autonumber
    participant App as Main App (IoTSentinel.ino)
    participant Lib as DeviceTelemetry Library
    participant Net as WiFiClientSecure (mbedTLS)
    participant Cloud as Google Apps Script (doPost)
    participant Drive as Google Drive Storage

    App->>Lib: sendEvent("led_change", wifiOk, rssi, ledState)
    Lib->>Lib: Serialize JSON payload via ArduinoJson
    Lib->>Net: Connect to script.google.com:443
    Net->>Net: Verify server cert against GTS Root R1 CA
    Net->>Cloud: HTTPS POST /macros/s/.../exec (JSON + Auth Token)
    
    activate Cloud
    Cloud->>Cloud: Validate payload size (< 32 KB)
    Cloud->>Cloud: Verify token against Script Properties
    Cloud->>Cloud: Strip token from payload object
    Cloud->>Cloud: Check event type against allowlist
    
    alt Event is Approved (startup, heartbeat, led_change)
        Cloud->>Drive: Create file "telemetry_..."
    else Event is Simulated Anomaly
        Cloud->>Cloud: Flag security alert in audit metadata
        Cloud->>Drive: Create file "ALERT_telemetry_..."
    end
    
    Cloud-->>Net: HTTP 302 Found (Location: script.googleusercontent.com/...)
    deactivate Cloud

    Note over Lib: Catch 302 redirect.<br/>Verify target is *.googleusercontent.com.<br/>Do NOT forward token.
    Lib->>Net: HTTPS GET script.googleusercontent.com/macros/echo?...
    Net-->>Lib: HTTP 200 OK {"status": "success", "file_id": "..."}
    Lib-->>App: TelemetryResult.success = true
```

---

## 4. Software Modularity & Architectural Isolation

In Arduino embedded development, C++ libraries run in the **same address space and privilege level** as the main application. There is no memory segmentation, user/kernel space separation, or capability-based security model.

* **SmartLED**: Pure hardware actuator abstraction. Encapsulates GPIO initialization, level inversion (active-high/low), and state queries. Contains no network dependencies.
* **DeviceTelemetry**: Application-layer network client. Encapsulates `WiFiClientSecure`, `HTTPClient`, and `ArduinoJson`. Enforces Google GTS Root R1 certificate validation and domain-validated redirect handling.
* **Main Application (`IoTSentinel.ino`)**: Coordinates state between the actuator and the telemetry engine.
