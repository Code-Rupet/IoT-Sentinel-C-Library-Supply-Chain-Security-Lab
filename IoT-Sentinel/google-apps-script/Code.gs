/**
 * ============================================================================
 * IoT Sentinel — Google Apps Script Research Receiver
 * ============================================================================
 * Acts as the serverless HTTPS endpoint for the C++ Library Supply-Chain Security Lab.
 * Ingests, validates, sanitizes, and archives telemetry events into Google Drive.
 *
 * Security & Research Features:
 *  - Token validation against encrypted Script Properties (never saved in Drive).
 *  - Strict payload size limits (max 32 KB) to prevent DoS.
 *  - Event type allowlist ('startup', 'test', 'heartbeat', 'led_change', 'simulated_anomaly').
 *  - Supply-chain anomaly detection: Tags 'simulated_anomaly' records with 'ALERT_' prefixes
 *    and attaches security audit metadata for the research pipeline.
 *  - Path-traversal proof filename sanitization.
 *  - Generic error responses without stack trace leaks.
 *  - Safe doGet() health check.
 */

// Configuration Constants
const MAX_PAYLOAD_BYTES = 32768; // 32 KB maximum request size
const ALLOWED_EVENTS = ['startup', 'test', 'heartbeat', 'led_change', 'simulated_anomaly', 'alert'];
const ALLOWED_LED_STATES = ['ON', 'OFF'];

/**
 * Handle incoming HTTPS GET requests (Health Check endpoint).
 */
function doGet(e) {
  const healthResponse = {
    status: 'healthy',
    service: 'IoT Sentinel Research Receiver',
    version: '1.1.0',
    timestamp_utc: new Date().toISOString()
  };
  return createJsonResponse(healthResponse, 200);
}

/**
 * Handle incoming HTTPS POST requests containing telemetry JSON.
 */
function doPost(e) {
  try {
    // 1. Validate request payload existence
    if (!e || !e.postData || !e.postData.contents) {
      return createJsonResponse({
        status: 'error',
        code: 'EMPTY_PAYLOAD',
        message: 'Request body is empty or missing.'
      }, 400);
    }

    // 2. Validate payload size
    const rawContent = e.postData.contents;
    if (rawContent.length > MAX_PAYLOAD_BYTES) {
      return createJsonResponse({
        status: 'error',
        code: 'PAYLOAD_TOO_LARGE',
        message: 'Request payload exceeds maximum permitted size (32 KB).'
      }, 413);
    }

    // 3. Parse JSON safely
    let payload;
    try {
      payload = JSON.parse(rawContent);
    } catch (parseErr) {
      return createJsonResponse({
        status: 'error',
        code: 'MALFORMED_JSON',
        message: 'Request body is not valid JSON.'
      }, 400);
    }

    // 4. Validate authentication token against Script Properties
    const scriptProperties = PropertiesService.getScriptProperties();
    const configuredToken = scriptProperties.getProperty('DEVICE_TOKEN');
    const folderId = scriptProperties.getProperty('DRIVE_FOLDER_ID');
    const expectedDeviceId = scriptProperties.getProperty('EXPECTED_DEVICE_ID');

    if (!configuredToken || !folderId) {
      Logger.log('ERROR: Receiver misconfigured. Missing DEVICE_TOKEN or DRIVE_FOLDER_ID in Script Properties.');
      return createJsonResponse({
        status: 'error',
        code: 'SERVER_MISCONFIGURED',
        message: 'Receiver configuration is incomplete. Contact the system administrator.'
      }, 500);
    }

    const incomingToken = payload.token || payload.device_token || (e.parameter ? e.parameter.token : null);
    if (!incomingToken || !constantTimeEquals(String(incomingToken), String(configuredToken))) {
      Logger.log('WARNING: Authentication failed for incoming request.');
      return createJsonResponse({
        status: 'error',
        code: 'UNAUTHORIZED',
        message: 'Invalid or missing authentication token.'
      }, 401);
    }

    // 5. SECURITY CRITICAL: Strip the token before saving to Google Drive!
    delete payload.token;
    delete payload.device_token;

    // 6. Validate Device ID
    if (!payload.device_id || typeof payload.device_id !== 'string') {
      return createJsonResponse({
        status: 'error',
        code: 'INVALID_DEVICE_ID',
        message: 'Field "device_id" is required and must be a string.'
      }, 400);
    }

    const deviceIdRegex = /^[a-zA-Z0-9_-]{1,64}$/;
    if (!deviceIdRegex.test(payload.device_id)) {
      return createJsonResponse({
        status: 'error',
        code: 'INVALID_DEVICE_ID_FORMAT',
        message: 'Field "device_id" contains invalid characters or invalid length.'
      }, 400);
    }

    if (expectedDeviceId && payload.device_id !== expectedDeviceId) {
      Logger.log('WARNING: Device ID mismatch. Received: ' + payload.device_id + ', Expected: ' + expectedDeviceId);
      return createJsonResponse({
        status: 'error',
        code: 'DEVICE_ID_MISMATCH',
        message: 'Device ID is not recognized.'
      }, 403);
    }

    // 7. Validate Event Type against Allowlist
    if (!payload.event || typeof payload.event !== 'string' || ALLOWED_EVENTS.indexOf(payload.event) === -1) {
      return createJsonResponse({
        status: 'error',
        code: 'INVALID_EVENT',
        message: 'Field "event" must be one of: ' + ALLOWED_EVENTS.join(', ')
      }, 400);
    }

    // 8. Validate Schema Fields
    if (typeof payload.firmware_version !== 'string' || payload.firmware_version.length > 32) {
      return createJsonResponse({
        status: 'error',
        code: 'INVALID_FIRMWARE_VERSION',
        message: 'Field "firmware_version" must be a string <= 32 characters.'
      }, 400);
    }

    if (typeof payload.uptime_seconds !== 'number' || payload.uptime_seconds < 0) {
      return createJsonResponse({
        status: 'error',
        code: 'INVALID_UPTIME',
        message: 'Field "uptime_seconds" must be a non-negative number.'
      }, 400);
    }

    if (typeof payload.wifi_connected !== 'boolean') {
      return createJsonResponse({
        status: 'error',
        code: 'INVALID_WIFI_STATUS',
        message: 'Field "wifi_connected" must be a boolean.'
      }, 400);
    }

    if (typeof payload.wifi_rssi !== 'number' || payload.wifi_rssi < -130 || payload.wifi_rssi > 10) {
      return createJsonResponse({
        status: 'error',
        code: 'INVALID_RSSI',
        message: 'Field "wifi_rssi" must be a valid dBm number between -130 and 10.'
      }, 400);
    }

    if (typeof payload.led_state !== 'string' || ALLOWED_LED_STATES.indexOf(payload.led_state) === -1) {
      return createJsonResponse({
        status: 'error',
        code: 'INVALID_LED_STATE',
        message: 'Field "led_state" must be "ON" or "OFF".'
      }, 400);
    }

    // 9. Supply-Chain Security Analysis: Check for Simulated Anomaly (Configuration C)
    const isAnomaly = (payload.event === 'simulated_anomaly' || payload.synthetic_anomaly_flag === true);
    if (isAnomaly) {
      Logger.log('SECURITY ALERT: Ingestion pipeline detected simulated anomalous telemetry event!');
    }

    // 10. Add Server-Side Metadata (UTC Receipt Timestamp & Audit Record)
    const serverTimestampUtc = new Date().toISOString();
    payload.server_received_at_utc = serverTimestampUtc;
    payload.security_audit = {
      anomaly_detected: isAnomaly,
      classification: isAnomaly ? 'SUPPLY_CHAIN_RESEARCH_ANOMALY' : 'NORMAL_TELEMETRY',
      payload_size_bytes: rawContent.length
    };

    // 11. Construct Safe Filename (prevent path traversal or collision)
    const safeDeviceId = payload.device_id.replace(/[^a-zA-Z0-9_-]/g, '_');
    const safeEvent = payload.event.replace(/[^a-zA-Z0-9_-]/g, '_');
    const fileTimestamp = serverTimestampUtc.replace(/[:.]/g, '-');
    const uniqueSuffix = Utilities.getUuid().substring(0, 8);
    const prefix = isAnomaly ? 'ALERT_telemetry_' : 'telemetry_';
    const filename = prefix + safeDeviceId + '_' + safeEvent + '_' + fileTimestamp + '_' + uniqueSuffix + '.json';

    // 12. Write JSON File to Google Drive Folder
    let targetFolder;
    try {
      targetFolder = DriveApp.getFolderById(folderId);
    } catch (folderErr) {
      Logger.log('ERROR: Unable to access Drive folder ID: ' + folderId + '. ' + folderErr.toString());
      return createJsonResponse({
        status: 'error',
        code: 'STORAGE_UNAVAILABLE',
        message: 'Target storage folder could not be accessed.'
      }, 500);
    }

    const jsonContent = JSON.stringify(payload, null, 2);
    const createdFile = targetFolder.createFile(filename, jsonContent, MimeType.PLAIN_TEXT);

    Logger.log('Saved telemetry record: ' + filename + ' (ID: ' + createdFile.getId() + ')');

    // 13. Return Success Confirmation
    return createJsonResponse({
      status: 'success',
      file_id: createdFile.getId(),
      filename: filename,
      anomaly_detected: isAnomaly,
      received_at: serverTimestampUtc
    }, 200);

  } catch (err) {
    Logger.log('UNCAUGHT_EXCEPTION: ' + err.toString());
    return createJsonResponse({
      status: 'error',
      code: 'INTERNAL_ERROR',
      message: 'An unexpected internal error occurred while processing the telemetry event.'
    }, 500);
  }
}

/**
 * Constant-time string comparison to mitigate timing attacks.
 */
function constantTimeEquals(a, b) {
  if (typeof a !== 'string' || typeof b !== 'string') {
    return false;
  }
  let diff = a.length ^ b.length;
  for (let i = 0; i < a.length; i++) {
    diff |= a.charCodeAt(i) ^ (i < b.length ? b.charCodeAt(i) : 0);
  }
  return diff === 0;
}

/**
 * Helper to produce standard JSON output for Google Apps Script Web Apps.
 */
function createJsonResponse(data, httpStatusCode) {
  const output = ContentService.createTextOutput(JSON.stringify(data));
  output.setMimeType(ContentService.MimeType.JSON);
  return output;
}
