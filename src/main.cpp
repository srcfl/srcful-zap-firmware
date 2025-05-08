#include "endpoints/endpoint_types.h"
#include <WiFi.h>
#include <ESPmDNS.h>
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
#include "backend/data_sender.h"
#include "data/data_reader_task.h"
#include "backend/backend_api_task.h" // Include BackendApiTask
#include "ota/ota_handler.h"  // Include OTA handler
#include "debug.h" // Include Debug header
#include "main_action_manager.h" // Include the action manager
#include "main_actions.h" // Include actions for triggering

#include "zap_log.h" // Include crypto functions

static const char* TAG = "main"; // Tag for logging


#define IO_BUTTON 9

// Global variables
BackendApiTask backendApiTask; // Create a backend API task instance
WifiManager wifiManager(MDNS_NAME); // Create a WiFi manager instance, pass mDNS name
WifiStatusTask wifiStatusTask; // Create a WiFi status task instance
DataReaderTask dataReaderTask; // Create a data reader task instance
ServerTask serverTask(80); // Create a server task instance

OTAHandler g_otaHandler;

// Instantiate MainActionManager directly
MainActionManager mainActionManager;

String configuredSSID = "";
String configuredPassword = "";

// Button press detection variables
unsigned long buttonPressStartTime = 0;
bool buttonPressed = false;
const unsigned long CLEAR_WIFI_PRESS_DURATION = 5000; // 5 seconds for long press
const unsigned long REBOOT_PRESS_DURATION = 2000; // 2 seconds for reboot

#include "ble_handler.h"
BLEHandler bleHandler; 
unsigned long lastBLECheck = 0;  // Track last BLE check time




void setup() {

    // --- Get and store the reset reason ---
    esp_reset_reason_t reason = esp_reset_reason();
    Debug::setResetReason(reason);

    Serial.begin(115200);
    delay(1000); // Give some time for serial connection but no loop as we are not using the serial port in the actual meters

    LOG_I(TAG, "\n\n--- Srcful ZAP Firmware Booting ---");

    pinMode(LED_PIN, OUTPUT);
    pinMode(IO_BUTTON, INPUT_PULLUP); // Initialize button pin with internal pull-up
    digitalWrite(LED_PIN, LOW); // Quick blink on startup
    delay(500);
    digitalWrite(LED_PIN, HIGH);


    LOG_I(TAG, "Starting setup...");

    LOG_I(TAG, "Total heap: %d\n", ESP.getHeapSize());
    LOG_I(TAG, "Free heap: %d\n", ESP.getFreeHeap());
    LOG_I(TAG, "Total PSRAM: %d\n", ESP.getPsramSize());
    LOG_I(TAG, "Free PSRAM: %d\n", ESP.getFreePsram());

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
            LOG_I(TAG, "No private key found in Preferences");
            uint8_t privateKey[32];
            if (crypto_create_private_key(privateKey)) {
                // convert to hex string
                char* privateKeyHex = new char [sizeof(privateKey) * 2 + 1];
                bytes_to_hex_string(privateKey, sizeof(privateKey), privateKeyHex);
                PRIVATE_KEY_HEX = privateKeyHex;
                
                // Store the private key in Preferences
                preferences.putBytes("private_key", privateKey, sizeof(privateKey));
            } else {
                LOG_E(TAG, "Failed to create private key!!");
            }
        }
        preferences.end();
        Serial.printf("serial number: %s\n", crypto_getId().c_str());
        Serial.printf("Public key: %s\n", crypto_get_public_key(PRIVATE_KEY_HEX).c_str());
    }
    

    // Try to connect using saved credentials first.
    // if there is no saved credentials we need to start the BLE
    if (!wifiManager.loadCredentials()) {
        LOG_I(TAG, "No saved credentials found, starting BLE setup...");
    
        bleHandler.init();
    } else {
        LOG_I(TAG, "Found saved credentials, attempting to connect...");
    }
    
    
    // Configure and start the WiFi status task
    wifiStatusTask.setWifiManager(&wifiManager);
    wifiStatusTask.setLedPin(LED_PIN);
    wifiStatusTask.begin();
    
    // Start the OTA handler
    // TODO: The endpoint should be passed to the OTA handler
    extern OTAHandler g_otaHandler;
    g_otaHandler.begin();
    
    // Configure and start the data sender task
    // dataSenderTask.begin(&wifiManager);
    // dataSenderTask.setInterval(5000); // 5 seconds interval for checking the queue
    // dataSenderTask.setBleActive(true);
    
    // Configure and start the data reader task
    dataReaderTask.setInterval(10000); // 10 seconds interval for generating data
    dataReaderTask.begin(backendApiTask.getQueueHandle()); // Share the queue between tasks
    
    // Start the backend API task
    backendApiTask.begin(&wifiManager);  // Pass the WiFi manager reference
    backendApiTask.setInterval(300000);  // 5 minutes interval (300,000 ms) for state updates
    backendApiTask.setBleActive(true);   // Initialize with BLE active (same as dataSenderTask)
    
    // Start the server task
    // server task is started/restarted in the main loop when wifi is connected
    // serverTask.begin();


    LOG_I(TAG, "Setup completed successfully!");
    LOG_I(TAG, "Free heap after setup: %i", ESP.getFreeHeap());
}

void loop() {
    // --- Check for deferred actions FIRST ---
    mainActionManager.checkAndExecute(wifiManager, backendApiTask); // Call method on the instance

    // Handle button press for WiFi reset
    int buttonState = digitalRead(IO_BUTTON);
    
    // Button is active LOW (pressed when reading LOW)
    if (buttonState == LOW) {
        // Button is currently pressed
        if (!buttonPressed) {
            // Button was just pressed, record the time and do a quick blink
            buttonPressStartTime = millis();
            buttonPressed = true;
            // LOG_V(TAG, "Button pressed, starting timer");
            
            // Quick LED blink as feedback
            digitalWrite(LED_PIN, LOW);
            delay(100);
            digitalWrite(LED_PIN, HIGH);
        } else {
            // Button is being held, check for long press
            unsigned long pressDuration = millis() - buttonPressStartTime;
            
            // When we reach 5 seconds threshold, start continuous fast blinking
            if (pressDuration > CLEAR_WIFI_PRESS_DURATION) {
                // Reached long press threshold - continuous fast blinking as feedback
                digitalWrite(LED_PIN, (pressDuration / 100) % 2 ? HIGH : LOW); // Fast blink (5Hz)
            } else if (pressDuration > REBOOT_PRESS_DURATION) {
                // Reached reboot threshold - slow blinking as feedback
                digitalWrite(LED_PIN, (pressDuration / 250) % 2 ? HIGH : LOW); // Slow blink (2Hz)
            }
        }
    } else if (buttonPressed) {
        // Button was released
        unsigned long pressDuration = millis() - buttonPressStartTime;
        buttonPressed = false;
        
        if (pressDuration > CLEAR_WIFI_PRESS_DURATION) {
            // Long press confirmed and button released, now reset WiFi and restart
            // LOG_V(TAG, "Long press confirmed! Resetting WiFi settings...");
            
            // Clear WiFi credentials
            wifiManager.clearCredentials();
        }

        // no else here as we want to reboot even if the button was pressed for less than 5 seconds
        // i.e. this is a reboot without wifi credentials clearing.
        if (pressDuration > REBOOT_PRESS_DURATION) {
            // Short press confirmed, trigger a reboot
            // LOG_V(TAG, "Short press confirmed! Rebooting...");
            
            // Trigger a deferred reboot using the new system (e.g., 1 second delay)
            MainActions::triggerAction(MainActions::Type::REBOOT, 10); // Request reboot in 1000ms
        }

         // Restore LED state after button release (if not rebooting)
         // This might be handled by wifiStatusTask anyway, but ensures LED is back on if connected.
         if (wifiManager.isConnected()) {
             digitalWrite(LED_PIN, HIGH);
         } else {
             // Or handle blinking via wifiStatusTask if not connected
         }
    }

    // Check if the server task is running
    if (wifiManager.isConnected()) {
        
        
        // Check if the server task is running, restart if needed
        if (!serverTask.isRunning()) {
            Serial.println("Server task not running, restarting...");
            serverTask.begin();
        }
    }

    if (!buttonPressed && bleHandler.isActive()) {
        // If BLE is active and button is not pressed, turn on the LED
        digitalWrite(LED_PIN, LOW);
    }
    
    #if defined(USE_BLE_SETUP)
    // Update BLE state in data sender task and backend API task
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

