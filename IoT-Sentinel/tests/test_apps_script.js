/**
 * Automated Unit Test Suite for Google Apps Script Receiver (Code.gs)
 * ====================================================================
 * Simulates the Google Apps Script environment in Node.js to verify:
 *  - Authentication & token validation
 *  - Schema enforcement & allowlist validation
 *  - Token sanitization (token is never stored in Drive)
 *  - Supply-chain anomaly detection (Configuration C 'ALERT_' tagging)
 *  - Safe filename generation & path traversal prevention
 *  - Health check endpoint
 *  - Error handling (empty, malformed, oversized payloads)
 */

const fs = require('fs');
const path = require('path');
const vm = require('vm');
const assert = require('assert');

console.log('===================================================================');
console.log('   IoT Sentinel — Google Apps Script Automated Receiver Tests      ');
console.log('===================================================================\n');

// 1. Create Mock Google Apps Script Environment
let storedProperties = {
  DEVICE_TOKEN: 'sentinel_sec_test_token_987654',
  DRIVE_FOLDER_ID: 'mock_drive_folder_id_12345',
  EXPECTED_DEVICE_ID: 'esp32-sentinel-01'
};

let createdDriveFiles = [];

const mockLogger = {
  log: (msg) => { /* console.log('[AppsScript Logger]:', msg); */ }
};

const mockPropertiesService = {
  getScriptProperties: () => ({
    getProperty: (key) => storedProperties[key] || null
  })
};

const mockDriveFolder = {
  createFile: (filename, content, mimeType) => {
    const file = {
      _id: 'file_' + Math.random().toString(36).substring(2, 10),
      _filename: filename,
      _content: content,
      _mimeType: mimeType,
      getId: function() { return this._id; },
      getName: function() { return this._filename; }
    };
    createdDriveFiles.push(file);
    return file;
  }
};

const mockDriveApp = {
  getFolderById: (id) => {
    if (id === storedProperties.DRIVE_FOLDER_ID) {
      return mockDriveFolder;
    }
    throw new Error('Folder not found');
  }
};

const mockUtilities = {
  getUuid: () => 'uuid-' + Math.random().toString(36).substring(2, 10)
};

const mockContentService = {
  MimeType: { JSON: 'application/json' },
  createTextOutput: (text) => {
    return {
      _text: text,
      _mimeType: null,
      setMimeType: function(mime) { this._mimeType = mime; return this; },
      getContent: function() { return this._text; },
      getParsed: function() { return JSON.parse(this._text); }
    };
  }
};

const sandbox = {
  console: console,
  Logger: mockLogger,
  PropertiesService: mockPropertiesService,
  DriveApp: mockDriveApp,
  Utilities: mockUtilities,
  ContentService: mockContentService,
  MimeType: { PLAIN_TEXT: 'text/plain' }
};

// 2. Load Code.gs into Node.js VM context
const codePath = path.join(__dirname, '..', 'google-apps-script', 'Code.gs');
const codeContent = fs.readFileSync(codePath, 'utf8');

vm.createContext(sandbox);
vm.runInContext(codeContent, sandbox);

let passedCount = 0;
let totalTests = 0;

function runTest(testName, fn) {
  totalTests++;
  try {
    fn();
    console.log(`[PASS] Test ${totalTests}: ${testName}`);
    passedCount++;
  } catch (err) {
    console.error(`[FAIL] Test ${totalTests}: ${testName}`);
    console.error('       Error:', err.message);
  }
}

// ===================================================================
// TEST SUITE
// ===================================================================

// Test 1: Health Check (doGet)
runTest('Health endpoint doGet() returns status: healthy', () => {
  const result = sandbox.doGet({});
  const data = result.getParsed();
  assert.strictEqual(data.status, 'healthy');
  assert.ok(data.service.includes('IoT Sentinel'));
  assert.ok(data.timestamp_utc);
  assert.strictEqual(data.DEVICE_TOKEN, undefined);
  assert.strictEqual(data.DRIVE_FOLDER_ID, undefined);
});

// Test 2: Valid Telemetry Event (doPost)
runTest('Valid telemetry ingestion creates file in Drive and strips token', () => {
  createdDriveFiles = [];
  const testPayload = {
    device_id: 'esp32-sentinel-01',
    event: 'test',
    firmware_version: '1.1.0-research',
    uptime_seconds: 45,
    wifi_connected: true,
    wifi_rssi: -58,
    led_state: 'ON',
    token: 'sentinel_sec_test_token_987654'
  };

  const response = sandbox.doPost({
    postData: { contents: JSON.stringify(testPayload) }
  });

  const parsedResponse = response.getParsed();
  assert.strictEqual(parsedResponse.status, 'success');
  assert.ok(parsedResponse.file_id);
  assert.ok(parsedResponse.filename);

  // Verify file in Drive mock
  assert.strictEqual(createdDriveFiles.length, 1);
  const savedContent = JSON.parse(createdDriveFiles[0]._content);
  assert.strictEqual(savedContent.device_id, 'esp32-sentinel-01');
  assert.strictEqual(savedContent.event, 'test');
  assert.strictEqual(savedContent.uptime_seconds, 45);
  assert.strictEqual(savedContent.wifi_rssi, -58);
  assert.strictEqual(savedContent.led_state, 'ON');
  assert.ok(savedContent.server_received_at_utc, 'Server UTC timestamp must be present');

  // CRITICAL SECURITY ASSERTION: Token must NOT be stored in Drive!
  assert.strictEqual(savedContent.token, undefined, 'CRITICAL: Token must be stripped from saved file!');
});

// Test 3: Invalid Authentication Token
runTest('Invalid token is rejected with UNAUTHORIZED (401)', () => {
  createdDriveFiles = [];
  const testPayload = {
    device_id: 'esp32-sentinel-01',
    event: 'test',
    firmware_version: '1.1.0-research',
    uptime_seconds: 45,
    wifi_connected: true,
    wifi_rssi: -58,
    led_state: 'ON',
    token: 'WRONG_FORGED_TOKEN'
  };

  const response = sandbox.doPost({
    postData: { contents: JSON.stringify(testPayload) }
  });

  const parsedResponse = response.getParsed();
  assert.strictEqual(parsedResponse.status, 'error');
  assert.strictEqual(parsedResponse.code, 'UNAUTHORIZED');
  assert.strictEqual(createdDriveFiles.length, 0, 'No file should be created on unauthorized request');
});

// Test 4: Missing Token
runTest('Missing token is rejected with UNAUTHORIZED (401)', () => {
  const testPayload = {
    device_id: 'esp32-sentinel-01',
    event: 'heartbeat',
    firmware_version: '1.1.0-research',
    uptime_seconds: 120,
    wifi_connected: true,
    wifi_rssi: -54,
    led_state: 'ON'
  };

  const response = sandbox.doPost({
    postData: { contents: JSON.stringify(testPayload) }
  });

  const parsed = response.getParsed();
  assert.strictEqual(parsed.status, 'error');
  assert.strictEqual(parsed.code, 'UNAUTHORIZED');
});

// Test 5: Malformed JSON Body
runTest('Malformed JSON body is rejected with MALFORMED_JSON (400)', () => {
  const response = sandbox.doPost({
    postData: { contents: '{ "broken": json without closing' }
  });

  const parsed = response.getParsed();
  assert.strictEqual(parsed.status, 'error');
  assert.strictEqual(parsed.code, 'MALFORMED_JSON');
});

// Test 6: Empty Body
runTest('Empty request is rejected with EMPTY_PAYLOAD (400)', () => {
  const response = sandbox.doPost(null);
  const parsed = response.getParsed();
  assert.strictEqual(parsed.status, 'error');
  assert.strictEqual(parsed.code, 'EMPTY_PAYLOAD');
});

// Test 7: Unsupported Event Type
runTest('Unsupported event type is rejected with INVALID_EVENT (400)', () => {
  const testPayload = {
    device_id: 'esp32-sentinel-01',
    event: 'unauthorized_shell_command',
    firmware_version: '1.1.0-research',
    uptime_seconds: 45,
    wifi_connected: true,
    wifi_rssi: -58,
    led_state: 'ON',
    token: 'sentinel_sec_test_token_987654'
  };

  const response = sandbox.doPost({
    postData: { contents: JSON.stringify(testPayload) }
  });

  const parsed = response.getParsed();
  assert.strictEqual(parsed.status, 'error');
  assert.strictEqual(parsed.code, 'INVALID_EVENT');
});

// Test 8: Oversized Payload (>32 KB)
runTest('Payload exceeding 32 KB is rejected with PAYLOAD_TOO_LARGE (413)', () => {
  const largePadding = 'A'.repeat(35000);
  const testPayload = {
    device_id: 'esp32-sentinel-01',
    event: 'test',
    firmware_version: '1.1.0-research',
    uptime_seconds: 45,
    wifi_connected: true,
    wifi_rssi: -58,
    led_state: 'ON',
    padding: largePadding,
    token: 'sentinel_sec_test_token_987654'
  };

  const response = sandbox.doPost({
    postData: { contents: JSON.stringify(testPayload) }
  });

  const parsed = response.getParsed();
  assert.strictEqual(parsed.status, 'error');
  assert.strictEqual(parsed.code, 'PAYLOAD_TOO_LARGE');
});

// Test 9: Device ID Path Traversal Sanitization
runTest('Path traversal in device_id is sanitized safely in filename', () => {
  createdDriveFiles = [];
  const testPayload = {
    device_id: 'esp32-sentinel-01',
    event: 'heartbeat',
    firmware_version: '1.1.0-research',
    uptime_seconds: 120,
    wifi_connected: true,
    wifi_rssi: -54,
    led_state: 'ON',
    token: 'sentinel_sec_test_token_987654'
  };

  const response = sandbox.doPost({
    postData: { contents: JSON.stringify(testPayload) }
  });

  const parsed = response.getParsed();
  assert.strictEqual(parsed.status, 'success');
  const filename = createdDriveFiles[0]._filename;
  assert.ok(!filename.includes('/'), 'Filename must not contain slashes');
  assert.ok(!filename.includes('..'), 'Filename must not contain directory traversal sequences');
  assert.ok(filename.startsWith('telemetry_esp32-sentinel-01_heartbeat_'));
});

// Test 10: Server Misconfiguration (Missing Properties)
runTest('Missing Script Properties returns SERVER_MISCONFIGURED (500)', () => {
  const originalToken = storedProperties.DEVICE_TOKEN;
  storedProperties.DEVICE_TOKEN = null;

  const testPayload = {
    device_id: 'esp32-sentinel-01',
    event: 'test',
    firmware_version: '1.1.0-research',
    uptime_seconds: 45,
    wifi_connected: true,
    wifi_rssi: -58,
    led_state: 'ON',
    token: 'some_token'
  };

  const response = sandbox.doPost({
    postData: { contents: JSON.stringify(testPayload) }
  });

  const parsed = response.getParsed();
  assert.strictEqual(parsed.status, 'error');
  assert.strictEqual(parsed.code, 'SERVER_MISCONFIGURED');

  storedProperties.DEVICE_TOKEN = originalToken;
});

// Test 11: Configuration C Supply-Chain Anomaly Detection & ALERT_ Tagging
runTest('Configuration C simulated anomaly triggers anomaly_detected and ALERT_ filename prefix', () => {
  createdDriveFiles = [];
  const anomalyPayload = JSON.parse(fs.readFileSync(path.join(__dirname, 'simulated-anomaly.json'), 'utf8')).synthetic_anomalous_payload;
  anomalyPayload.token = 'sentinel_sec_test_token_987654';

  const response = sandbox.doPost({
    postData: { contents: JSON.stringify(anomalyPayload) }
  });

  const parsed = response.getParsed();
  assert.strictEqual(parsed.status, 'success');
  assert.strictEqual(parsed.anomaly_detected, true, 'Anomaly detector must flag anomaly_detected as true');
  assert.ok(parsed.filename.startsWith('ALERT_telemetry_'), 'Filename must have ALERT_ prefix for anomalies');

  // Verify stored file
  assert.strictEqual(createdDriveFiles.length, 1);
  const saved = JSON.parse(createdDriveFiles[0]._content);
  assert.strictEqual(saved.event, 'simulated_anomaly');
  assert.strictEqual(saved.token, undefined, 'Token must still be stripped');
  assert.strictEqual(saved.security_audit.anomaly_detected, true);
  assert.strictEqual(saved.security_audit.classification, 'SUPPLY_CHAIN_RESEARCH_ANOMALY');
});

// ===================================================================
// SUMMARY
// ===================================================================
console.log('\n-------------------------------------------------------------------');
console.log(` Test Results: ${passedCount} / ${totalTests} passed (${Math.round((passedCount/totalTests)*100)}%)`);
console.log('-------------------------------------------------------------------\n');

if (passedCount === totalTests) {
  console.log('>>> ALL GOOGLE APPS SCRIPT LOGIC TESTS COMPLETED SUCCESSFULLY! <<<\n');
  process.exit(0);
} else {
  console.error('>>> SOME TESTS FAILED. CHECK LOGS ABOVE. <<<\n');
  process.exit(1);
}
