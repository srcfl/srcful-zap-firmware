#include "wifi_manager.h"
#include "config.h"

WifiManager::WifiManager() 
    : _isProvisioned(false), 
      _lastScanTime(0) {
}

WifiManager::~WifiManager() {
    // Cleanup if needed
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

bool WifiManager::connectToWiFi(const String& ssid, const String& password, bool updateGlobals) {
    if (ssid.length() > 0 && password.length() > 0) {
        Serial.println("Connecting to WiFi...");
        Serial.print("SSID: ");
        Serial.println(ssid);
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
        std::vector<String> uniqueSSIDs;
        for (int i = 0; i < n; ++i) {
            String ssid = WiFi.SSID(i);
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

String WifiManager::getLocalIP() const {
    return WiFi.localIP().toString();
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