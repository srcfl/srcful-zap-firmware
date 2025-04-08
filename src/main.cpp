#include "endpoints/endpoint_types.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include "html.h"
#include "crypto.h"
#include <HTTPClient.h>
#include <esp_system.h>
#include "p1data.h"
#include "endpoints/endpoint_mapper.h"
#include <WiFiClientSecure.h>
#include "esp_heap_caps.h"
#include "graphql.h"
#include "config.h"
#include "server/server_task.h"
#include "wifi/wifi_manager.h"
#include "wifi/wifi_status_task.h"

#define LED_PIN 7

// Global variables
ServerTask serverTask(80); // Create a server task instance
WifiManager wifiManager; // Create a WiFi manager instance
WifiStatusTask wifiStatusTask; // Create a WiFi status task instance
bool isProvisioned = false;
String configuredSSID = "";
String configuredPassword = "";
unsigned long lastJWTTime = 0;
const unsigned long JWT_INTERVAL = 10000; // 10 seconds in milliseconds
unsigned long bleShutdownTime = 0; // Time when BLE should be shut down (0 = no shutdown scheduled)

#if defined(USE_BLE_SETUP)
    #include "ble_handler.h"
    BLEHandler bleHandler;  // Remove parentheses to make it an object, not a function
    bool isBleActive = false;  // Track BLE state
#endif

// Function declarations
void setupSSL();

// Add hardcoded WiFi credentials


void setup() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW); // Quick blink on startup
    delay(500);
    digitalWrite(LED_PIN, HIGH);



    Serial.begin(115200);

    Serial.printf("Total heap: %d\n", ESP.getHeapSize());
    Serial.printf("Free heap: %d\n", ESP.getFreeHeap());
    Serial.printf("Total PSRAM: %d\n", ESP.getPsramSize());
    Serial.printf("Free PSRAM: %d\n", ESP.getFreePsram());
    
    // Initialize SSL early
    initSSL();
    

    
    #if defined(DIRECT_CONNECT)
        // Connect to WiFi directly
        Serial.println("Connecting to WiFi...");
        WiFi.mode(WIFI_STA);
        if (wifiManager.connectToWiFi(WIFI_SSID, WIFI_PSK)) {
            digitalWrite(LED_PIN, HIGH); // Solid LED when connected
        } else {
            Serial.println("WiFi connection failed");
            digitalWrite(LED_PIN, LOW);
        }
    #endif
    
    
    // Perform initial WiFi scan
    Serial.println("Starting WiFi scan...");
    wifiManager.scanWiFiNetworks();
    Serial.println("WiFi scan completed");
    
    // Run signing test
    
    // Continue with normal setup
    #if defined(USE_SOFTAP_SETUP)
        Serial.println("Setting up AP mode...");
        wifiManager.setupAP(AP_SSID, AP_PASSWORD);
    #endif
    
    #if defined(USE_BLE_SETUP)
        Serial.println("Setting up BLE...");
        bleHandler.init();
        isBleActive = true;  // Set BLE active flag
    #endif
    
    #if defined(DIRECT_CONNECT)
        Serial.println("Using direct connection mode");
    #endif

    #if !defined(USE_SOFTAP_SETUP) && !defined(USE_BLE_SETUP) && !defined(DIRECT_CONNECT)
        Serial.println("ERROR: No connection mode defined!");
        #error "Must define either USE_SOFTAP_SETUP, USE_BLE_SETUP, or DIRECT_CONNECT"
    #endif
    
    Serial.println("Setting up MDNS...");
    if (wifiManager.setupMDNS(MDNS_NAME)) {
        Serial.println("MDNS responder started");
    } else {
        Serial.println("Error setting up MDNS responder!");
    }
    
    // Configure and start the WiFi status task
    wifiStatusTask.setWifiManager(&wifiManager);
    wifiStatusTask.setLedPin(LED_PIN);
    wifiStatusTask.setJwtInterval(JWT_INTERVAL);
    wifiStatusTask.setLastJWTTime(lastJWTTime);
    wifiStatusTask.setBleShutdownTime(bleShutdownTime);
    wifiStatusTask.setBleActive(isBleActive);
    wifiStatusTask.begin();
    
    // Start the server task
    Serial.println("Starting server task...");
    serverTask.begin();
    Serial.println("Server task started");
    
    Serial.println("Setup completed successfully!");
    Serial.print("Free heap after setup: ");
    Serial.println(ESP.getFreeHeap());
}

void loop() {
    // Check if the server task is running
    if (wifiManager.isConnected()) {
        digitalWrite(LED_PIN, LOW); // Solid LED when connected
        
        // Check if the server task is running, restart if needed
        if (!serverTask.isRunning()) {
            Serial.println("Server task not running, restarting...");
            serverTask.begin();
        }
    }
    
    yield();
}

