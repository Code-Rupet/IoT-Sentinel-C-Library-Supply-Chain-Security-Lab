#ifndef SECRETS_H
#define SECRETS_H

/**
 * IoT Sentinel - Secrets Configuration Template
 * =============================================
 * Instructions:
 * 1. Duplicate this file and rename it to 'secrets.h' in the same directory:
 *      cp secrets.example.h secrets.h
 * 2. Populate 'secrets.h' with your local network and Google deployment details.
 * 3. Never commit 'secrets.h' to Git (it is listed in .gitignore).
 *
 * Cybersecurity Advisory (University Lab Note):
 * ---------------------------------------------
 * A pre-shared token compiled directly into firmware provides basic authentication
 * against unauthorized callers on the network. However, anyone with physical access
 * to an unencrypted ESP32 can dump the SPI flash memory (via esptool.py) and
 * extract these string constants.
 *
 * In production enterprise environments, secrets should be stored in a dedicated
 * Hardware Secure Element (e.g., Microchip ATECC608A) or protected using ESP32
 * Flash Encryption and Secure Boot v2.
 */

// Local Wi-Fi network credentials
#define SECRET_WIFI_SSID     "YOUR_WIFI_SSID"
#define SECRET_WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// Google Apps Script Web App Deployment URL
// Example: "https://script.google.com/macros/s/AKfycbx...ABC123xyz/exec"
#define SECRET_GOOGLE_SCRIPT_URL "https://script.google.com/macros/s/YOUR_DEPLOYMENT_ID/exec"

// Shared Device Authentication Token
// Must match the 'DEVICE_TOKEN' property configured in Google Apps Script Script Properties
#define SECRET_DEVICE_TOKEN "YOUR_STRONG_RANDOM_DEVICE_TOKEN"

#endif // SECRETS_H
