# Threat Model — C++ IoT Library Supply Chains

This document presents a cybersecurity threat model for microcontroller software supply chains, focusing on C++ embedded development on the ESP32 platform. It applies the **STRIDE** methodology and aligns with **NIST SP 800-161** (*Cybersecurity Supply Chain Risk Management*).

---

## 1. The Embedded C++ Execution Reality

In modern desktop and cloud environments, processes operate within virtual memory spaces enforced by hardware Memory Management Units (MMUs), operating system rings, and container sandboxes.

In embedded microcontroller systems (e.g., standard ESP32 development under Arduino/FreeRTOS):
* **Flat Address Space**: The entire physical SRAM, memory-mapped flash cache, and hardware peripherals share a single, unprotected memory map.
* **No Privilege Separation**: All code—whether written by the primary developer or imported from an external library—runs with supervisor privileges.
* **Implicit Execution Vectors in C++**:
  1. **Static / Global Constructors**: Code in global class constructors or functions decorated with `__attribute__((constructor))` executes **before** `setup()` is entered.
  2. **FreeRTOS Background Tasks**: Any library can call `xTaskCreatePinnedToCore()` during initialization to spawn independent background threads that run concurrently without the developer's knowledge.
  3. **Interrupt Hooking**: A library can attach hardware timer or GPIO interrupts (`attachInterrupt()`) to gain recurring execution slices.

```
+-------------------------------------------------------------------------------+
|                    ESP32 UNIFIED SRAM & PERIPHERAL BUS                        |
+-------------------------------------------------------------------------------+
| Main Application Code | Third-Party Libraries | Wi-Fi Buffers | Flash Cache   |
| (IoTSentinel.ino)     | (SmartLED, Telemetry) | (SRAM)        | (secrets.h)   |
+-------------------------------------------------------------------------------+
|               NO MEMORY ISOLATION • ALL CODE RUNS WITH SUPERVISOR ACCESS     |
+-------------------------------------------------------------------------------+
```

---

## 2. Developer Mental Model vs. Runtime Execution Reality

| Dimension | Developer Mental Model | Runtime Execution Reality |
| :--- | :--- | :--- |
| **Execution Trigger** | "The library only runs when I call its functions in my code." | Global constructors or FreeRTOS tasks execute automatically upon boot. |
| **Resource Access** | "The library only interacts with the GPIO pin I pass to it." | The library has direct pointer access to all RAM, Wi-Fi radios, and flash. |
| **Network Destination**| "Network traffic only goes to the URL I configure." | A library can make independent outbound socket connections to any IP address. |
| **Trust Anchor** | "If the library is published on GitHub, it has been reviewed." | Most open-source libraries undergo no security auditing before inclusion. |

---

## 3. STRIDE Threat Analysis Matrix

| Threat Category | Supply-Chain Attack Vector | Embedded Manifestation | Mitigation in IoT Sentinel |
| :--- | :--- | :--- | :--- |
| **Spoofing** | Attacker impersonates a legitimate library author (typo-squatting). | Developer installs `DeviceTelemety` instead of `DeviceTelemetry`. | Pinned dependency inventory with exact repository URLs and SHA-256 baselines. |
| **Tampering** | Upstream repository compromised; malicious commits pushed to release tags. | Library source code modified to inject unapproved network beacons. | Cryptographic hashing (`sha256sum`) of library source files prior to compilation. |
| **Repudiation** | Undocumented telemetry emitted without audit records. | Developer cannot verify whether unexpected traffic originated from device or network. | Server-side UTC receipt timestamps and audit classification metadata. |
| **Information Disclosure** | Library scrapes SRAM or flash to harvest Wi-Fi passwords and tokens. | Pre-shared secrets embedded in flash dumped or transmitted over network. | Strict data minimization, TLS certificate validation, token redaction in logs. |
| **Denial of Service** | Library triggers infinite loops or memory exhaustion (heap fragmentation). | Microcontroller watchdog timer (WDT) trips, causing continuous boot loops. | Non-blocking execution loops, fixed-size static buffers, timeout safeguards. |
| **Elevation of Privilege** | Library hijacks execution flow or reconfigures hardware registers. | Actuator (LED or relay) toggled without application consent. | Strict code review of library source files for direct register manipulation. |

---

## 4. Supply-Chain Attack Lifecycle (Embedded Context)

```
[Phase 1: Infiltration]
    Upstream maintainer account compromised OR typo-squatted library published
           │
           ▼
[Phase 2: Acquisition]
    Developer downloads library into Arduino IDE / libraries folder
           │
           ▼
[Phase 3: Compilation & Linking]
    Compiler links third-party object code into monolithic firmware image (.bin)
           │
           ▼
[Phase 4: Flashing & Execution]
    Firmware executes on ESP32 with unrestricted access to memory and network hardware
           │
           ▼
[Phase 5: Side-Effect Activation]
    Compromised library emits unexpected telemetry OR manipulates hardware state
```

---

## 5. Defense-in-Depth Countermeasures

1. **Pre-Build Verification**: Review library source code for `#include <WiFi.h>`, `WiFiClient`, `HTTPClient`, raw sockets (`lwip`), and FreeRTOS task spawning (`xTaskCreate`).
2. **Integrity Pinning**: Record and compare SHA-256 hashes of all third-party source files before every build.
3. **Egress Network Monitoring**: Validate that network traffic matches the documented baseline using Wireshark packet captures.
4. **Cloud-Side Anomaly Detection**: Reject or flag unapproved event types, unexpected keys, and out-of-schedule transmissions in the ingestion pipeline.
