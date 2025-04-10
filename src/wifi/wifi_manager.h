#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <ESPmDNS.h>
#include <vector>
#include <algorithm>
#include <Arduino.h>
#include <Preferences.h>  // Add Preferences library for NVS access

class WifiManager {
public:
    WifiManager();
    ~WifiManager();

    // Setup methods
    void setupAP(const char* ssid, const char* password);
    bool connectToWiFi(const String& ssid, const String& password, bool updateGlobals = true);
    void scanWiFiNetworks();
    
    // Status methods
    virtual bool isConnected() const;
    String getLocalIP() const;
    int getStatus() const;
    
    // Getters for stored data
    bool isProvisioned() const { return _isProvisioned; }
    virtual String getConfiguredSSID() const { return _configuredSSID; }
    String getConfiguredPassword() const { return _configuredPassword; }
    virtual const std::vector<String>& getLastScanResults() const { return _lastScanResults; }
    unsigned long getLastScanTime() const { return _lastScanTime; }
    
    // Setters
    void setProvisioned(bool provisioned) { _isProvisioned = provisioned; }
    void setConfiguredSSID(const String& ssid) { _configuredSSID = ssid; }
    void setConfiguredPassword(const String& password) { _configuredPassword = password; }
    
    // MDNS setup
    bool setupMDNS(const char* hostname);
    
    // Persistence methods
    bool loadCredentials();  // Load credentials from NVS
    bool saveCredentials();  // Save credentials to NVS
    bool clearCredentials(); // Clear saved credentials
    
    // Automatic connection using saved credentials
    bool autoConnect();

private:
    void initNTP();

    // Member variables
    bool _isProvisioned;
    String _configuredSSID;
    String _configuredPassword;
    std::vector<String> _lastScanResults;
    unsigned long _lastScanTime;
    Preferences _preferences;  // Preferences instance for NVS operations
    static const unsigned long SCAN_CACHE_TIME = 10000; // Cache scan results for 10 seconds
    
    // Constants for NVS storage
    static const char* PREF_NAMESPACE;
    static const char* KEY_SSID;
    static const char* KEY_PASSWORD;
    static const char* KEY_PROVISIONED;
};

#endif // WIFI_MANAGER_H