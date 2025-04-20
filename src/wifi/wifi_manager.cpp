#include "wifi_manager.h"
#include "config.h"

// Define static constants for NVS storage
const char* WifiManager::PREF_NAMESPACE = "wificonfig";
const char* WifiManager::KEY_SSID = "ssid";
const char* WifiManager::KEY_PASSWORD = "password";
const char* WifiManager::KEY_PROVISIONED = "provisioned";

WifiManager::WifiManager() 
    : _isProvisioned(false), 
      _lastScanTime(0) {
    
    Serial.println("Initializing WiFi Manager...");
    
    // Try to load credentials at initialization
    // Begin with the namespace but don't end it - we'll keep it open throughout the lifecycle
    if (_preferences.begin(PREF_NAMESPACE, true)) {
        _isProvisioned = _preferences.getBool(KEY_PROVISIONED, false);
        
        if (_isProvisioned) {

            _configuredSSID = getString(_preferences, KEY_SSID, "");
            _configuredPassword = getString(_preferences, KEY_PASSWORD, "");;
            
            Serial.println("WiFi credentials loaded from NVS");
            Serial.print("SSID: ");
            Serial.println(_configuredSSID.c_str());
            Serial.print("Password length: ");
            Serial.println(_configuredPassword.length());
        } else {
            Serial.println("No saved WiFi credentials found");
        }
        
        _preferences.end(); // Close after reading initial values
    } else {
        Serial.println("Failed to open preferences namespace");
    }
}

WifiManager::~WifiManager() {
    // Nothing special needed for cleanup
    // Preferences automatically closes when goes out of scope
}

void WifiManager::setupAP(const char* ssid, const char* password) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, password);
    
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
}

void WifiManager::initNTP() {
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");  // 0, 0 = UTC, no daylight offset
    
    Serial.print("Waiting for NTP time sync: ");
    time_t now = time(nullptr);
    while (now < 8 * 3600 * 2) {
        delay(500);
        Serial.print(".");
        now = time(nullptr);
    }
    Serial.println();
}

bool WifiManager::connectToWiFi(const zap::Str& ssid, const zap::Str& password, bool updateGlobals) {
    if (ssid.length() > 0 && password.length() > 0) {
        Serial.println("Connecting to WiFi...");
        Serial.print("SSID: ");
        Serial.println(ssid.c_str());
        Serial.print("Password length: ");
        Serial.println(password.length());
        
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
            Serial.print(".");
            attempts++;
        }
        Serial.println();
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("WiFi connected");
            Serial.print("IP address: ");
            Serial.println(WiFi.localIP());
            
            // Initialize NTP time synchronization
            Serial.println("Initializing NTP...");
            initNTP();
            Serial.print("NTP initialized, Epoch:");
            Serial.println(time(nullptr));
            
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
            
            return true;
        } else {
            Serial.println("WiFi connection failed");
            WiFi.disconnect(true);  // Clean disconnect on failure
            return false;
        }
    } else {
        Serial.println("No WiFi credentials provided");
        return false;
    }
}

void WifiManager::scanWiFiNetworks() {
    Serial.println("Scanning WiFi networks...");
    
    // Set WiFi to station mode to perform scan
    WiFi.mode(WIFI_STA);
    
    int n = WiFi.scanNetworks();
    Serial.println("Scan completed");
    
    _lastScanResults.clear();
    
    if (n == 0) {
        Serial.println("No networks found");
    } else {
        Serial.print(n);
        Serial.println(" networks found");
        
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
    
    // Return to original mode if we were in AP mode
    if (_isProvisioned) {
        WiFi.mode(WIFI_STA);
    } else {
        setupAP(AP_SSID, AP_PASSWORD);
    }
}

bool WifiManager::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

zap::Str WifiManager::getLocalIP() const {
    return WiFi.localIP().toString().c_str();
}

int WifiManager::getStatus() const {
    return WiFi.status();
}

bool WifiManager::setupMDNS(const char* hostname) {
    if (MDNS.begin(hostname)) {
        Serial.println("MDNS responder started");
        return true;
    } else {
        Serial.println("Error setting up MDNS responder!");
        return false;
    }
}

bool WifiManager::loadCredentials() {
    Serial.println("Loading WiFi credentials from NVS...");
    
    if (!_preferences.begin(PREF_NAMESPACE, true)) { // true = read-only
        Serial.println("Failed to open NVS namespace");
        return false;
    }
    
    // Read values from NVS
    _isProvisioned = _preferences.getBool(KEY_PROVISIONED, false);
    Serial.print("Loaded isProvisioned: ");
    Serial.println(_isProvisioned ? "true" : "false");
    
    if (_isProvisioned) {
        _configuredSSID = getString(_preferences, KEY_SSID, "");
        _configuredPassword = getString(_preferences, KEY_PASSWORD, "");
        
        Serial.println("Credentials loaded successfully");
        Serial.print("SSID: ");
        Serial.println(_configuredSSID.c_str());
        Serial.print("Password length: ");
        Serial.println(_configuredPassword.length());
        
        // Additional debug info about what was actually read
        Serial.print("Raw SSID from NVS: '");
        Serial.print(_preferences.getString(KEY_SSID, "<not found>"));
        Serial.println("'");
        
        // Check if the namespace contains the expected keys
        Serial.print("NVS contains SSID key: ");
        Serial.println(_preferences.isKey(KEY_SSID) ? "yes" : "no");
        Serial.print("NVS contains PASSWORD key: ");
        Serial.println(_preferences.isKey(KEY_PASSWORD) ? "yes" : "no");
    } else {
        Serial.println("No saved credentials found (_isProvisioned flag is false)");
    }
    
    _preferences.end();
    return _isProvisioned;
}

bool WifiManager::saveCredentials() {
    Serial.println("Saving WiFi credentials to NVS...");
    
    if (!_preferences.begin(PREF_NAMESPACE, false)) { // false = read-write
        Serial.println("Failed to open NVS namespace");
        return false;
    }
    
    // Print current values being saved
    Serial.print("Saving isProvisioned: ");
    Serial.println(_isProvisioned ? "true" : "false");
    
    if (_isProvisioned) {
        Serial.print("Saving SSID: '");
        Serial.print(_configuredSSID.c_str());
        Serial.println("'");
        Serial.print("Saving Password (length): ");
        Serial.println(_configuredPassword.length());
    }
    
    // Save values to NVS
    bool provResult = _preferences.putBool(KEY_PROVISIONED, _isProvisioned);
    Serial.print("Result of saving isProvisioned: ");
    Serial.println(provResult ? "success" : "failure");
    
    bool ssidResult = false;
    bool pwdResult = false;
    
    if (_isProvisioned) {
        ssidResult = _preferences.putString(KEY_SSID, _configuredSSID.c_str());
        pwdResult = _preferences.putString(KEY_PASSWORD, _configuredPassword.c_str());
        
        Serial.print("Result of saving SSID: ");
        Serial.println(ssidResult ? "success" : "failure");
        Serial.print("Result of saving password: ");
        Serial.println(pwdResult ? "success" : "failure");
        
        // Verify the data was saved correctly
        Serial.print("Verifying saved SSID: '");
        Serial.print(_preferences.getString(KEY_SSID, "<not found>"));
        Serial.println("'");
        Serial.print("Verifying saved password length: ");
        Serial.println(_preferences.getString(KEY_PASSWORD, "").length());
    }
    
    _preferences.end();
    
    if (!_isProvisioned || (ssidResult && pwdResult && provResult)) {
        Serial.println("Credentials saved successfully");
        return true;
    } else {
        Serial.println("Failed to save one or more credential values");
        return false;
    }
}

bool WifiManager::clearCredentials() {
    Serial.println("Clearing saved WiFi credentials...");
    
    if (!_preferences.begin(PREF_NAMESPACE, false)) { // false = read-write
        Serial.println("Failed to open NVS namespace");
        return false;
    }
    
    // Clear all saved values
    _preferences.clear();
    _preferences.end();
    
    // Also clear in-memory values
    _isProvisioned = false;
    _configuredSSID = "";
    _configuredPassword = "";
    
    Serial.println("Credentials cleared successfully");
    return true;
}

bool WifiManager::autoConnect() {
    Serial.println("Attempting to auto-connect to WiFi...");
    
    // Force reload credentials from NVS to ensure we have the latest
    loadCredentials();
    
    // Try to connect using saved credentials
    if (_isProvisioned && _configuredSSID.length() > 0) {
        Serial.print("Found saved credentials for SSID: ");
        Serial.println(_configuredSSID.c_str());
        Serial.print("Password length: ");
        Serial.println(_configuredPassword.length());
        
        return connectToWiFi(_configuredSSID, _configuredPassword, false);
    }
    
    Serial.println("No saved credentials to auto-connect");
    return false;
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