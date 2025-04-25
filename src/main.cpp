#include "endpoints/endpoint_types.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include "html.h"
#include "crypto.h"
#include <HTTPClient.h>
#include <esp_system.h>
#include "endpoints/endpoint_mapper.h"
#include <WiFiClientSecure.h>
#include "esp_heap_caps.h"
#include "backend/graphql.h"  // Updated path to graphql.h
#include "config.h"
#include "server/server_task.h"
#include "wifi/wifi_manager.h"
#include "wifi/wifi_status_task.h"
#include "data/data_sender_task.h"
#include "data/data_reader_task.h"
#include "backend/backend_api_task.h" // Include BackendApiTask
#include "ota_handler.h"  // Include OTA handler

#define LED_PIN 3
#define IO_BUTTON 9

// Global variables
ServerTask serverTask(80); // Create a server task instance
WifiManager wifiManager; // Create a WiFi manager instance
WifiStatusTask wifiStatusTask; // Create a WiFi status task instance
DataSenderTask dataSenderTask; // Create a data sender task instance
DataReaderTask *dataReaderTask; // Create a data reader task instance
BackendApiTask backendApiTask; // Create a backend API task instance

String configuredSSID = "";
String configuredPassword = "";

// Button press detection variables
unsigned long buttonPressStartTime = 0;
bool buttonPressed = false;
const unsigned long LONG_PRESS_DURATION = 5000; // 5 seconds for long press

#if defined(USE_BLE_SETUP)
    #include "ble_handler.h"
    BLEHandler bleHandler; 
    unsigned long lastBLECheck = 0;  // Track last BLE check time
#endif


void setup() {
    pinMode(LED_PIN, OUTPUT);
    pinMode(IO_BUTTON, INPUT_PULLUP); // Initialize button pin with internal pull-up
    digitalWrite(LED_PIN, LOW); // Quick blink on startup
    delay(500);
    digitalWrite(LED_PIN, HIGH);

    Serial.begin(115200);
    delay(1000);
    Serial.println("Starting setup...");

    Serial.printf("Total heap: %d\n", ESP.getHeapSize());
    Serial.printf("Free heap: %d\n", ESP.getFreeHeap());
    Serial.printf("Total PSRAM: %d\n", ESP.getPsramSize());
    Serial.printf("Free PSRAM: %d\n", ESP.getFreePsram());

    {   // get the private key from the Preferences
        Preferences preferences;
        preferences.begin("crypto", false);
        uint8_t privateKeyBytes[32];
        
        if (preferences.getBytes("private_key", privateKeyBytes, sizeof(privateKeyBytes)) == sizeof(privateKeyBytes)) {
            // convert to hex string
            char* privateKeyHex = new char [sizeof(privateKeyBytes) * 2 + 1];
            bytes_to_hex_string(privateKeyBytes, sizeof(privateKeyBytes), privateKeyHex);
            PRIVATE_KEY_HEX = privateKeyHex;
        } else {
            Serial.println("No private key found in Preferences");
            uint8_t privateKey[32];
            if (crypto_create_private_key(privateKey)) {
                // convert to hex string
                char* privateKeyHex = new char [sizeof(privateKey) * 2 + 1];
                bytes_to_hex_string(privateKey, sizeof(privateKey), privateKeyHex);
                PRIVATE_KEY_HEX = privateKeyHex;
                
                // Store the private key in Preferences
                preferences.putBytes("private_key", privateKey, sizeof(privateKey));
            } else {
                Serial.println("Failed to create private key");
            }
        }
        preferences.end();
        Serial.printf("serial number: %s\n", crypto_getId().c_str());
        Serial.printf("Public key: %s\n", crypto_get_public_key(PRIVATE_KEY_HEX).c_str());
    }
    
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
    #else
        // Try to connect using saved credentials first
        if (!wifiManager.autoConnect()) {
            Serial.println("No saved credentials or connection failed");
            
            // Fall back to regular setup modes if auto-connect fails
            #if defined(USE_SOFTAP_SETUP)
                Serial.println("Setting up AP mode...");
                wifiManager.setupAP(AP_SSID, AP_PASSWORD);
            #endif
            
            #if defined(USE_BLE_SETUP)
                Serial.println("Setting up BLE...");
                bleHandler.init();
            #endif
        } else {
            Serial.println("Successfully connected with saved credentials");
            digitalWrite(LED_PIN, HIGH); // Solid LED when connected
        }
    #endif
    
    // Perform initial WiFi scan
    Serial.println("Starting WiFi scan...");
    wifiManager.scanWiFiNetworks();
    Serial.println("WiFi scan completed");
    
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
    wifiStatusTask.begin();
    
    // Start the OTA handler
    Serial.println("Starting OTA handler...");
    extern OTAHandler g_otaHandler;
    g_otaHandler.begin();
    
    // Configure and start the data sender task
    dataSenderTask.begin(&wifiManager);
    dataSenderTask.setInterval(5000); // 5 seconds interval for checking the queue
    dataSenderTask.setBleActive(true);
    
    // Configure and start the data reader task
    dataReaderTask = new DataReaderTask();
    dataReaderTask->setInterval(10000); // 10 seconds interval for generating data
    dataReaderTask->begin(dataSenderTask.getQueueHandle()); // Share the queue between tasks
    
    // Start the backend API task
    Serial.println("Starting backend API task...");
    backendApiTask.begin(&wifiManager);  // Pass the WiFi manager reference
    backendApiTask.setInterval(300000);  // 5 minutes interval (300,000 ms) for state updates
    backendApiTask.setBleActive(true);   // Initialize with BLE active (same as dataSenderTask)
    Serial.println("Backend API task started");
    
    // Start the server task
    Serial.println("Starting server task...");
    serverTask.begin();
    Serial.println("Server task started");
    
    Serial.println("Setup completed successfully!");
    Serial.print("Free heap after setup: ");
    Serial.println(ESP.getFreeHeap());
}

void loop() {
    // Handle button press for WiFi reset
    int buttonState = digitalRead(IO_BUTTON);
    
    // Button is active LOW (pressed when reading LOW)
    if (buttonState == LOW) {
        // Button is currently pressed
        if (!buttonPressed) {
            // Button was just pressed, record the time and do a quick blink
            buttonPressStartTime = millis();
            buttonPressed = true;
            Serial.println("Button pressed, starting timer");
            
            // Quick LED blink as feedback
            digitalWrite(LED_PIN, LOW);
            delay(100);
            digitalWrite(LED_PIN, HIGH);
        } else {
            // Button is being held, check for long press
            unsigned long pressDuration = millis() - buttonPressStartTime;
            
            // When we reach 5 seconds threshold, start continuous fast blinking
            if (pressDuration > LONG_PRESS_DURATION) {
                // Reached long press threshold - continuous fast blinking as feedback
                digitalWrite(LED_PIN, (pressDuration / 100) % 2 ? HIGH : LOW); // Fast blink (5Hz)
            }
        }
    } else if (buttonPressed) {
        // Button was released
        unsigned long pressDuration = millis() - buttonPressStartTime;
        buttonPressed = false;
        
        if (pressDuration > LONG_PRESS_DURATION) {
            // Long press confirmed and button released, now reset WiFi and restart
            Serial.println("Long press confirmed! Resetting WiFi settings...");
            
            // Clear WiFi credentials
            wifiManager.clearCredentials();
            
            // Disconnect from WiFi
            WiFi.disconnect(true);
            WiFi.mode(WIFI_OFF);
            delay(500);
            
            #if defined(USE_BLE_SETUP)
                // Initialize BLE for fresh setup
                Serial.println("Initializing BLE...");
                bleHandler.init();
            #endif
            
            // Restart the ESP32
            Serial.println("Restarting the board...");
            delay(250);
            ESP.restart();
        }
    }

    // Check if the server task is running
    if (wifiManager.isConnected()) {
        digitalWrite(LED_PIN, HIGH); // Solid LED when connected
        
        // Check if the server task is running, restart if needed
        if (!serverTask.isRunning()) {
            Serial.println("Server task not running, restarting...");
            serverTask.begin();
        }
    }
    
    #if defined(USE_BLE_SETUP)
    // Update BLE state in data sender task and backend API task
    dataSenderTask.setBleActive(bleHandler.isActive());
    backendApiTask.setBleActive(bleHandler.isActive());
    
    // Handle BLE request queue in the main loop
    if (millis() - lastBLECheck > 1000) {
        lastBLECheck = millis();
        bleHandler.handlePendingRequest();
    }

    if (bleHandler.shouldHardStop(3000)) {
        bleHandler.hardStop();
    }
    #endif
    
    yield();
}

