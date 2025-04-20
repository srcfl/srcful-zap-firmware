#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <ESPmDNS.h>
#include <vector>
#include <algorithm>
#include <Preferences.h>  // Add Preferences library for NVS access
#include "zap_str.h"

class WifiManager {
public:
    WifiManager();
    ~WifiManager();

    // Setup methods
    void setupAP(const char* ssid, const char* password);
    bool connectToWiFi(const zap::Str& ssid, const zap::Str& password, bool updateGlobals = true);
    void scanWiFiNetworks();
    
    // Status methods
    virtual bool isConnected() const;
    zap::Str getLocalIP() const;
    int getStatus() const;
    
    // Getters for stored data
    bool isProvisioned() const { return _isProvisioned; }
    virtual zap::Str getConfiguredSSID() const { return _configuredSSID; }
    zap::Str getConfiguredPassword() const { return _configuredPassword; }
    virtual const std::vector<zap::Str>& getLastScanResults() const { return _lastScanResults; }
    unsigned long getLastScanTime() const { return _lastScanTime; }
    
    // Setters
    void setProvisioned(bool provisioned) { _isProvisioned = provisioned; }
    void setConfiguredSSID(const zap::Str& ssid) { _configuredSSID = ssid; }
    void setConfiguredPassword(const zap::Str& password) { _configuredPassword = password; }
    
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

    const zap::Str& getString(Preferences &pref, const char* key, const char* defaultValue = "");

    // Member variables
    bool _isProvisioned;
    zap::Str _configuredSSID;
    zap::Str _configuredPassword;
    std::vector<zap::Str> _lastScanResults;
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