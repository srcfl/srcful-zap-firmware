#include "wifi_manager.h"
#include "config.h"
#include "zap_log.h"

const char* TAG = "wifi_manager";  // Tag for logging

// Define static constants for NVS storage
// DO NOT CHANGE THESE or the NVS data will be lost
const char* WifiManager::PREF_NAMESPACE = "wificonfig";
const char* WifiManager::KEY_SSID = "ssid";
const char* WifiManager::KEY_PASSWORD = "password";
const char* WifiManager::KEY_PROVISIONED = "provisioned";

WifiManager::WifiManager(const char* mdnsHostname) 
    : _isProvisioned(false), 
      _lastScanTime(0),
      _mdnsHostname(mdnsHostname),
      _scanWiFiNetworks(true) {
    
    LOG_D(TAG, "Initializing WiFi Manager...");
    
    loadCredentials();  // Load saved credentials from NVS
}

WifiManager::~WifiManager() {
    // Nothing special needed for cleanup
    // Preferences automatically closes when goes out of scope
}

void WifiManager::initNTP() {
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");  // 0, 0 = UTC, no daylight offset
    
    LOG_D(TAG, "Waiting for NTP time sync: ");
    time_t now = time(nullptr);
    while (now < 8 * 3600 * 2) {
        delay(500);
        now = time(nullptr);
    }
    LOG_D(TAG, "NTP time sync complete");
}

bool WifiManager::connectToWiFi(const zap::Str& ssid, const zap::Str& password, bool updateGlobals) {
    if (ssid.length() > 0 && password.length() > 0) {
        _connectToWiFiProcessing = true;

        LOG_D(TAG, "Connecting to WiFi...");
        LOG_D(TAG, "SSID: %s", ssid.c_str());
        LOG_D(TAG, "Password length: %i", password.length());
        
        // First disconnect and wait a bit
        WiFi.disconnect(true);  // true = disconnect and clear settings
        delay(1000);  // Give it time to complete
        
        // Set mode and wait
        WiFi.mode(WIFI_STA);
        delay(100);
        
        // Start connection
        WiFi.begin(ssid.c_str(), password.c_str());
        
        // Wait for connection with longer timeout
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 30) {  // Increased timeout to 15 seconds
            delay(500);
            attempts++;
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            LOG_I(TAG, "WiFi connected");
            LOG_I(TAG, "IP address: %s", WiFi.localIP().toString().c_str());
            
            // Initialize NTP time synchronization
            LOG_I(TAG, "Initializing NTP...");
            initNTP();
            LOG_I(TAG, "NTP initialized");

            // Setup mDNS now that we are connected
            LOG_I(TAG, "Setting up MDNS...");
            if (setupMDNS(_mdnsHostname)) {
                 LOG_I(TAG, "MDNS responder started");
            } else {
                 LOG_E(TAG, "Error setting up MDNS responder!");
            }
            
            // Configure low power WiFi
            WiFi.setSleep(true);  // Enable modem sleep
            
            // Update global variables if requested
            if (updateGlobals) {
                _configuredSSID = ssid;
                _configuredPassword = password;
                _isProvisioned = true;
                
                // Save to persistent storage
                saveCredentials();
            }
            _connectToWiFiProcessing = true;
            return true;
        } else {
            LOG_W(TAG, "WiFi connection failed");
            WiFi.disconnect(true);  // Clean disconnect on failure
            _connectToWiFiProcessing = true;
            return false;
        }
    } else {
        LOG_W(TAG, "No WiFi credentials provided");
        _connectToWiFiProcessing = true;
        return false;
    }
}

void WifiManager::scanWiFiNetworks() {
    if (_connectToWiFiProcessing) {
        LOG_W(TAG, "Cannot scan WiFi networks while connecting to WiFi");
        return;
    }

    LOG_I(TAG, "Scanning WiFi networks...");
    
    // Set WiFi to station mode to perform scan
    WiFiMode_t currentMode = WiFi.getMode(); // Store current mode
    if (currentMode != WIFI_STA && currentMode != WIFI_AP_STA) {
        WiFi.mode(WIFI_STA);
        delay(100); // Give mode change time
    }
    
    int n = WiFi.scanNetworks();
    LOG_I(TAG, "Scan completed");
    
    _lastScanResults.clear();
    
    if (n == 0) {
        LOG_I(TAG, "No networks found");
    } else {
        LOG_I(TAG, "%d networks found", n);
        
        // Store unique SSIDs (some networks might broadcast on multiple channels)
        std::vector<zap::Str> uniqueSSIDs;
        for (int i = 0; i < n; ++i) {
            zap::Str ssid = WiFi.SSID(i).c_str();
            if (std::find(uniqueSSIDs.begin(), uniqueSSIDs.end(), ssid) == uniqueSSIDs.end()) {
                uniqueSSIDs.push_back(ssid);
            }
        }
        
        // Sort alphabetically
        std::sort(uniqueSSIDs.begin(), uniqueSSIDs.end());
        
        // Store in member vector
        _lastScanResults = uniqueSSIDs;
    }
    
    _lastScanTime = millis();
    
    // Restore original mode if it wasn't STA or AP_STA
    if (currentMode != WIFI_STA && currentMode != WIFI_AP_STA) {
         WiFi.mode(currentMode); 
         delay(100);
    } else if (currentMode == WIFI_MODE_NULL) {
         // Decide desired state - leaving STA for now if it was off.
    }
}

bool WifiManager::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

zap::Str WifiManager::getLocalIP() const {
    return WiFi.localIP().toString().c_str();
}

zap::Str WifiManager::getMacAddress() const {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    
    // Convert MAC address to string
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", 
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    return zap::Str(macStr);
}

int WifiManager::getStatus() const {
    return WiFi.status();
}

bool WifiManager::setupMDNS(const char* hostname) {
    if (MDNS.begin(hostname)) {
        return true;
    } else {
        return false;
    }
}

bool WifiManager::loadCredentials() {
    LOG_I(TAG, "Loading WiFi credentials from NVS...");
    
    if (!_preferences.begin(PREF_NAMESPACE, true)) { // true = read-only
        LOG_E(TAG, "Failed to open NVS namespace for reading");
        return false;
    }
    
    // Read values from NVS
    _isProvisioned = _preferences.getBool(KEY_PROVISIONED, false);
    LOG_I(TAG, "Loaded isProvisioned: %s", _isProvisioned ? "true" : "false");
    
    if (_isProvisioned) {
        _configuredSSID = getString(_preferences, KEY_SSID, "");
        _configuredPassword = getString(_preferences, KEY_PASSWORD, "");
        
        LOG_I(TAG, "Credentials loaded successfully");
        LOG_I(TAG, "SSID: %s", _configuredSSID.c_str());
        LOG_I(TAG, "Password length: %d", _configuredPassword.length());
        
        // Additional debug info about what was actually read
        LOG_D(TAG, "Raw SSID from NVS: '%s'", _preferences.getString(KEY_SSID, "<not found>").c_str());
        
        // Check if the namespace contains the expected keys
        LOG_D(TAG, "NVS contains SSID key: %s", _preferences.isKey(KEY_SSID) ? "yes" : "no");
        LOG_D(TAG, "NVS contains PASSWORD key: %s", _preferences.isKey(KEY_PASSWORD) ? "yes" : "no");
    } else {
        LOG_I(TAG, "No saved credentials found (_isProvisioned flag is false)");
    }
    
    _preferences.end();
    return _isProvisioned;
}

bool WifiManager::saveCredentials() {
    LOG_I(TAG, "Saving WiFi credentials to NVS...");
    
    if (!_preferences.begin(PREF_NAMESPACE, false)) { // false = read-write
        LOG_E(TAG, "Failed to open NVS namespace for writing");
        return false;
    }
    
    // Print current values being saved
    LOG_I(TAG, "Saving isProvisioned: %s", _isProvisioned ? "true" : "false");
    
    if (_isProvisioned) {
        LOG_I(TAG, "Saving SSID: '%s'", _configuredSSID.c_str());
        LOG_I(TAG, "Saving Password (length): %d", _configuredPassword.length());
    }
    
    // Save values to NVS
    bool provResult = _preferences.putBool(KEY_PROVISIONED, _isProvisioned);
    LOG_D(TAG, "Result of saving isProvisioned: %s", provResult ? "success" : "failure");
    
    bool ssidResult = false;
    bool pwdResult = false;
    
    if (_isProvisioned) {
        ssidResult = _preferences.putString(KEY_SSID, _configuredSSID.c_str());
        pwdResult = _preferences.putString(KEY_PASSWORD, _configuredPassword.c_str());
        
        LOG_D(TAG, "Result of saving SSID: %s", ssidResult ? "success" : "failure");
        LOG_D(TAG, "Result of saving password: %s", pwdResult ? "success" : "failure");
        
        // Verify the data was saved correctly
        LOG_D(TAG, "Verifying saved SSID: '%s'", _preferences.getString(KEY_SSID, "<not found>").c_str());
        LOG_D(TAG, "Verifying saved password length: %d", _preferences.getString(KEY_PASSWORD, "").length());
    }
    
    _preferences.end();
    
    if (!_isProvisioned || (ssidResult && pwdResult && provResult)) {
        LOG_I(TAG, "Credentials saved successfully");
        return true;
    } else {
        LOG_E(TAG, "Failed to save one or more credential values");
        return false;
    }
}

bool WifiManager::clearCredentials() {
    LOG_I(TAG, "Clearing saved WiFi credentials...");
    
    if (!_preferences.begin(PREF_NAMESPACE, false)) { // false = read-write
        LOG_E(TAG, "Failed to open NVS namespace for clearing");
        return false;
    }
    
    // Clear all saved values
    _preferences.clear();
    _preferences.end();
    
    // Also clear in-memory values
    _isProvisioned = false;
    _configuredSSID = "";
    _configuredPassword = "";
    
    LOG_I(TAG, "Credentials cleared successfully");
    return true;
}

bool WifiManager::autoConnect() {
    if (_connectToWiFiProcessing) {
        LOG_W(TAG, "Cannot scan WiFi networks while connecting to WiFi");
        return true;
    }

    LOG_I(TAG, "Attempting to auto-connect to WiFi...");
    
    // Force reload credentials from NVS to ensure we have the latest
    loadCredentials();
    
    bool connected = false;
    // Try to connect using saved credentials
    if (_isProvisioned && _configuredSSID.length() > 0) {
        LOG_I(TAG, "Found saved credentials for SSID: %s", _configuredSSID.c_str());
        
        connected = connectToWiFi(_configuredSSID, _configuredPassword, false); // Attempt connection
    } else {
         LOG_I(TAG, "No saved credentials found.");
    }
    
    // If connection failed or no credentials existed, perform a scan
    if (connected) {
        LOG_I(TAG, "Auto-connect successful.");

        // two quick blinks to indicate success
        digitalWrite(LED_PIN, HIGH);    // Turn off
        delay(150);
        digitalWrite(LED_PIN, LOW);
        delay(150);
        digitalWrite(LED_PIN, HIGH);
        delay(150);
        digitalWrite(LED_PIN, LOW);
        delay(150);
        digitalWrite(LED_PIN, HIGH); // Turn off

    }

    return connected; // Return true only if connection succeeded
}

bool WifiManager::disconnect() {
    LOG_I(TAG, "Disconnecting from WiFi...");
    
    if (WiFi.status() == WL_CONNECTED) {
        WiFi.disconnect(true);  // true = disconnect and clear settings
        delay(1000);  // Give it time to complete
        LOG_I(TAG, "Disconnected from WiFi");
        return true;
    } else {
        LOG_I(TAG, "Not connected to WiFi");
        return false;
    }
}

const zap::Str& WifiManager::getString(Preferences &pref, const char* key, const char* defaultValue) {
    static zap::Str result;
    char buffer[64] = {0};
    if (pref.getString(key, buffer, sizeof(buffer)) > 0) {
        result = buffer;
    } else {
        result = defaultValue;
    }
    
    return result;
}