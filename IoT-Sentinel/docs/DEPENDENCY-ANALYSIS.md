# Dependency Integrity & Source Review — IoT Sentinel

This document defines the software inventory, source code auditing methodology, and cryptographic integrity verification workflow for the **IoT Sentinel** project.

---

## 1. Software Dependency Inventory

| Component | Category | Version | Author / Maintainer | Source Location | Scope & Justification |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **`SmartLED`** | Custom / Local Library | `1.0.0` | IoT Sentinel Research Group | `libraries/SmartLED/` | Hardware LED actuator abstraction. Zero network dependencies. |
| **`DeviceTelemetry`** | Custom / Local Library | `1.0.0` | IoT Sentinel Research Group | `libraries/DeviceTelemetry/` | Application-level telemetry serialization, TLS client, and Google redirect engine. |
| **`ArduinoJson`** | Third-Party Library | `^7.0.0` | Benoit Blanchon | Arduino Library Manager / GitHub | Document object model and serialization for JSON telemetry. |
| **`WiFi`** | ESP32 Core SDK | `3.0.x` | Espressif Systems | Built into Espressif ESP32 Core | Layer 2/3 802.11 association, DHCP client, and TCP/IP stack (`lwIP`). |
| **`WiFiClientSecure`** | ESP32 Core SDK | `3.0.x` | Espressif Systems / mbedTLS | Built into Espressif ESP32 Core | Cryptographic engine for TLS 1.2/1.3 handshakes and certificate validation. |
| **`HTTPClient`** | ESP32 Core SDK | `3.0.x` | Espressif Systems | Built into Espressif ESP32 Core | Application-layer HTTP client and header parser. |

---

## 2. Arduino / C++ Embedded Code Review Checklist

Before integrating any third-party library into an embedded project, conduct a structured source audit:

### Checklist 1: Network & Socket Capabilities
- [ ] **Check for Network Includes**: Does the library `#include <WiFi.h>`, `<WiFiClient.h>`, `<HTTPClient.h>`, `<Ethernet.h>`, or `<lwip/sockets.h>`?
  - *Risk*: A library claiming to handle data formatting or sensor reading has no legitimate architectural need for network headers.
- [ ] **Scan for Hardcoded Endpoints**: Search all source and header files for string literals containing `http://`, `https://`, IP addresses, or `.com`/`.io`/`.xyz` domains.
  - *Command*: `grep -rnEI "https?://|[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}" libraries/`

### Checklist 2: Execution Vectors & Background Tasks
- [ ] **Inspect Global/Static Constructors**: Check for non-trivial logic executed inside class constructors declared in the global scope.
  - *Risk*: Global constructors run prior to `setup()`, executing before developer logging or safety controls are initialized.
- [ ] **Search for FreeRTOS Task Creation**: Look for calls to `xTaskCreate` or `xTaskCreatePinnedToCore`.
  - *Risk*: Spawns concurrent background threads that can initiate network connections or modify hardware state independently of `loop()`.
- [ ] **Inspect Hardware Interrupts**: Look for `attachInterrupt()` or hardware timer configurations (`timerBegin`, `esp_timer_create`).

### Checklist 3: Obfuscation & Encoded Payloads
- [ ] **Search for Obfuscated Byte Arrays**: Look for large arrays of hex bytes (`0x4a, 0x6f...`) or Base64-encoded strings that are decoded at runtime.
- [ ] **Check Pre-Compiled Binary Archives**: Ensure the library does not link pre-compiled static archives (`.a` or `.ar` files) in an `extra_flags` or `src/` subfolder whose source cannot be inspected.

---

## 3. Cryptographic Integrity Baseline (SHA-256)

The following cryptographic digests represent the verified baseline for the project source tree:

| File Path | SHA-256 Cryptographic Hash |
| :--- | :--- |
| `libraries/SmartLED/src/SmartLED.h` | `727ea9a2e9549b20b7195d2c65f6d77f216c536b8a27975d9c5096986731a935` |
| `libraries/SmartLED/src/SmartLED.cpp` | `671e28b083c37d51e3429d80046add249ef5eb48decfee2ad84111533630c4a8` |
| `libraries/DeviceTelemetry/src/DeviceTelemetry.h` | `172ee79ce5b72ccfb35a0167a2a95aed50aa575a64281fecb67f27c3efd46fec` |
| `libraries/DeviceTelemetry/src/DeviceTelemetry.cpp` | `e2d9fd43b58b5d1987cf5d7e22af1bf90166e953a68510ba35f9a6a08c055e6f` |
| `firmware/IoTSentinel/IoTSentinel.ino` | `6fa950dd482ab94f8e8ec40385bbca51954ae932d14135ef056d44d456712a2f` |
| `firmware/IoTSentinel/config.h` | `bd6e0fe1a685459fdd57431738f8a251888064943375972c872f4b69d977fe88` |
| `firmware/IoTSentinel/secrets.example.h` | `ac78db6edbdce4aa3156e52814a4f9da880e9dbb03735b9b75306a9f49d43550` |

### Hash Verification Command
Run the following command in your terminal to verify source tree integrity:
```bash
sha256sum libraries/SmartLED/src/* libraries/DeviceTelemetry/src/* firmware/IoTSentinel/*
```

---

## 4. Methodological Interpretation of Hash Mismatches

### Why a Hash Mismatch is an Investigation Trigger, Not Proof of Malice
When an automated build or audit pipeline detects that a library's hash differs from the baseline, it indicates that **the file content has changed**. It does **not** prove malicious intent:
1. **Benign Causes**:
   - An upstream developer released a bug fix, performance patch, or typo correction.
   - Line-ending conversions (e.g. CRLF on Windows vs. LF on Linux/macOS).
   - Comment or whitespace adjustments.
2. **Malicious Causes**:
   - Unauthorized code injection, backdooring, or dependency tampering.

**Engineering Protocol**: A hash mismatch must halt automated deployment and trigger a manual diff investigation:
```bash
diff -u baseline_file.cpp candidate_file.cpp
```

### The "Many Eyes" Fallacy: Why Open Source Does Not Equal Safe
A common assumption among embedded developers is that open-source libraries are safe because their code is public. Historical security incidents (e.g., *XZ Utils* backdooring in 2024, *Log4Shell* in 2021) demonstrate that:
* Most open-source libraries have few active maintainers and receive no routine security code reviews.
* Malicious contributions or subtle vulnerabilities can reside in plain sight for months or years.
* Developers must take active ownership of every external line of code compiled into their firmware images.
