# Network Traffic Analysis — IoT Sentinel

This document details the network analysis methodology using **Wireshark** to examine the packet emissions of the ESP32 under the three research configurations.

---

## 1. Network Capture Architecture

To inspect packets emitted by an embedded device without installing software on the microcontroller:

```
+-------------------+              +-------------------------------+
| ESP32 Dev Board   |  802.11 b/g  | Wi-Fi AP / Hotspot / Bridge   |
| (IP: 192.168.1.50)| ===========> | (Running Wireshark on Host)   |
+-------------------+              +-------------------------------+
                                                  |
                                                  v
                                     [Packet Capture Stream]
                                     Filter: ip.addr == 192.168.1.50
```

### Setup Methods
1. **Host-Based Software AP / Hotspot (Recommended)**: Enable a Wi-Fi hotspot on your research laptop and connect the ESP32. Capture traffic on the virtual hotspot interface.
2. **Promiscuous Mode on Monitor Interface**: Configure a wireless card in monitor mode to capture 802.11 frames over the air (requires WPA2 pre-shared key decryption in Wireshark).
3. **Ethernet Bridge with Port Mirroring**: Connect an external AP to an enterprise managed switch and mirror traffic to your analysis workstation.

---

## 2. Wireshark Inspection Methodology

### Essential Display Filters

| Analysis Objective | Wireshark Display Filter | What to Look For |
| :--- | :--- | :--- |
| **All ESP32 Activity** | `ip.addr == <ESP32_IP>` | Overall packet sequencing and bandwidth. |
| **DNS Resolutions** | `ip.addr == <ESP32_IP> && udp.port == 53` | Hostname queries (`pool.ntp.org`, `script.google.com`). Verify no unauthorized domains. |
| **Clock Sync (NTP)** | `ip.addr == <ESP32_IP> && udp.port == 123` | SNTP query and response packets with external time servers. |
| **Encrypted Transit** | `ip.addr == <ESP32_IP> && tcp.port == 443` | TLS 1.2/1.3 handshakes, TCP stream establishment, and cadence. |
| **TLS Handshake Inspection**| `tls.handshake.type == 1` | `Client Hello` packet containing the Server Name Indication (`tls.handshake.extension.server_name`). |

---

## 3. Cryptographic Visibility & Limits of TLS Inspection

When evaluating IoT telemetry over HTTPS, researchers must recognize what transport-layer encryption hides and what it reveals:

### What HTTPS / TLS Encrypts (Invisible to Wire Inspection)
* **Application JSON Payload**: The actual sensor readings, state values, and device identifiers.
* **Authentication Tokens**: The shared secret token (`token`).
* **HTTP Headers & Methods**: Whether the device is performing a `POST` or `GET`.
* **URL Paths & Query Parameters**: The specific deployment path (`/macros/s/.../exec`).

### What Remains Visible (Metadata & Side-Channels)
1. **Destination Hostname (SNI)**: In TLS 1.2 and standard TLS 1.3 without Encrypted Client Hello (ECH), the `Client Hello` packet contains the plaintext domain name in the SNI field (e.g., `script.google.com` or `script.googleusercontent.com`).
2. **Destination IP & Autonomous System (AS)**: Public routing information reveals who owns the receiving cloud infrastructure.
3. **Transmission Cadence & Timing**: Periodic bursts at exact 60,000 ms intervals reveal heartbeat mechanisms.
4. **Packet Size Signatures**: The length of TLS application data records correlates directly with the JSON payload size (e.g. ~350–550 bytes per event).

---

## 4. Multi-Configuration Comparative Analysis

The following table summarizes the expected versus observed network behavior across the three laboratory configurations:

| Configuration | Expected Behavior | Observed Network Behavior | Wireshark Evidence | Investigation Outcome |
| :--- | :--- | :--- | :--- | :--- |
| **Configuration A: Baseline** | Device connects to Wi-Fi; LED toggles locally; **zero** telemetry emitted. | DHCP exchange, NTP clock synchronization. **Zero** TCP port 443 packets emitted on LED toggle. | `ip.addr == ESP32 && tcp.port == 443` produces 0 packets during 10-minute test. | **PASS**: Baseline confirmed. Actuator logic is completely decoupled from network stack. |
| **Configuration B: Approved Telemetry** | Device emits startup event, 60s heartbeats, and instantaneous events on LED toggle. | Periodic TLS handshakes to `script.google.com` and `script.googleusercontent.com` at 60s intervals. | Clean `Client Hello` packets to Google SNI; TLS records sized ~400 bytes; HTTP 302 redirect followed. | **PASS**: Legitimate telemetry operates strictly within documented parameters and destinations. |
| **Configuration C: Simulated Compromised Dependency** | Device emits approved events **plus** an unexpected second telemetry packet upon actuator trigger. | Toggling the LED triggers **two consecutive TLS handshakes** within ~500 ms instead of one. | Two independent TCP connections to port 443 observed in Wireshark; Drive receives `ALERT_` prefixed file. | **ANOMALY DETECTED**: Network cadence anomaly identified via packet timing; payload audited via cloud logs. |

---

## 5. Detecting Unapproved Network Activity (Investigation Checklist)

When analyzing an unfamiliar IoT device or third-party library, follow this diagnostic checklist:

1. [ ] **Step 1: Check DNS Queries**: Are there lookups for domains not listed in the device's documentation?
   - *Red Flag*: Lookups for dynamic DNS providers (e.g. `duckdns.org`, `no-ip.biz`) or raw IP address connections.
2. [ ] **Step 2: Check Connection Destinations**: Do all HTTPS connections terminate at approved cloud infrastructure (e.g., Google, AWS, Azure)?
3. [ ] **Step 3: Analyze Timing Correlations**: Does a physical user action (e.g., button press or actuator toggle) generate unexplained outbound network bursts?
4. [ ] **Step 4: Check Unencrypted Traffic**: Does the device emit any plaintext HTTP (port 80) or UDP traffic that could leak device identifiers or local network topology?
