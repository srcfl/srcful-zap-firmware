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
#include "backend/backend_api_task.h"
#include "ota/ota_handler.h"
#include "debug.h"
#include "main_action_manager.h"
#include "main_actions.h" 
#include "ble/ble_handler.h"
#include "mqtt/mqtt_client.h"
#include "json_light/json_light.h"

#include "zap_log.h"

static constexpr LogTag TAG = LogTag("main", ZLOG_LEVEL_INFO);

// Function to generate authentication JWT for MQTT
String generateAuthJWT(const String& deviceId) {
    LOG_TI(TAG, "Starting JWT generation for device: %s", deviceId.c_str());
    
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t currentEpochSec = (uint64_t)(tv.tv_sec);
    uint64_t expirationTime = currentEpochSec + 10; // 10 seconds from now
    
    // Generate random JTI (using millis and device specific data for uniqueness)
    char jtiBuffer[25];
    snprintf(jtiBuffer, sizeof(jtiBuffer), "%08lx%08lx", 
             (unsigned long)(millis() & 0xFFFFFFFF), 
             (unsigned long)(ESP.getEfuseMac() & 0xFFFFFFFF));
    
    LOG_TI(TAG, "Generated JTI: %s, exp: %llu", jtiBuffer, (unsigned long long)expirationTime);
    LOG_TI(TAG, "Free heap before JSON building: %d", ESP.getFreeHeap());
    
    // Use JsonBuilder like data_sender.cpp does - dynamic version
    JsonBuilder headerBuilder;
    headerBuilder.beginObject()
        .add("alg", "ES256")
        .add("typ", "JWT")
        .add("device", deviceId.c_str())
        .add("opr", "production");
    zap::Str headerStr = headerBuilder.end();
    
    LOG_TI(TAG, "Free heap after header building: %d", ESP.getFreeHeap());
    
    JsonBuilder payloadBuilder;
    payloadBuilder.beginObject()
        .add("exp", expirationTime)
        .add("jti", jtiBuffer);
    zap::Str payloadStr = payloadBuilder.end();
    
    LOG_TI(TAG, "Free heap after payload building: %d", ESP.getFreeHeap());
    LOG_TI(TAG, "JWT Header: %s", headerStr.c_str());
    LOG_TI(TAG, "JWT Payload: %s", payloadStr.c_str());
    
    // Check if PRIVATE_KEY_HEX is valid before calling crypto
    if (!PRIVATE_KEY_HEX || strlen(PRIVATE_KEY_HEX) == 0) {
        LOG_TE(TAG, "PRIVATE_KEY_HEX is null or empty!");
        return "";
    }
    
    LOG_TI(TAG, "Private key length: %d", strlen(PRIVATE_KEY_HEX));
    LOG_TI(TAG, "Free heap before crypto_create_jwt: %d", ESP.getFreeHeap());
    
    // Sign and generate JWT
    zap::Str jwt = crypto_create_jwt(headerStr.c_str(), payloadStr.c_str(), PRIVATE_KEY_HEX);
    
    LOG_TI(TAG, "Free heap after crypto_create_jwt: %d", ESP.getFreeHeap());
    
    if (jwt.length() == 0) {
        LOG_TE(TAG, "Failed to create authentication JWT");
        return "";
    }
    
    LOG_TI(TAG, "JWT length: %d", jwt.length());
    LOG_TI(TAG, "Generated JWT: %s", jwt.c_str());
    LOG_TI(TAG, "Free heap before String conversion: %d", ESP.getFreeHeap());
    
    // Convert to String more safely
    String result;
    result.reserve(jwt.length() + 10); // Pre-allocate memory
    result = jwt.c_str();
    
    LOG_TI(TAG, "Generated auth JWT with exp: %llu, jti: %s", (unsigned long long)expirationTime, jtiBuffer);
    LOG_TI(TAG, "Result String length: %d", result.length());
    return result;
}


#define IO_BUTTON 9

// Global variables
BackendApiTask backendApiTask; // Create a backend API task instance
WifiManager wifiManager(MDNS_NAME); // Create a WiFi manager instance, pass mDNS name
WifiStatusTask wifiStatusTask; // Create a WiFi status task instance
DataReaderTask g_dataReaderTask; // Create a data reader task instance
ServerTask serverTask(80); // Create a server task instance

OTAHandler g_otaHandler;

// Instantiate MainActionManager directly
MainActionManager mainActionManager;

// MQTT Client instance with larger stack for SSL operations
ZapMQTTClient mqttClient(1024 * 12); // 12KB stack for SSL handshakes

String configuredSSID = "";
String configuredPassword = "";

// Button press detection variables
unsigned long buttonPressStartTime = 0;
bool buttonPressed = false;
const unsigned long CLEAR_WIFI_PRESS_DURATION = 5000; // 5 seconds for long press
const unsigned long REBOOT_PRESS_DURATION = 2000; // 2 seconds for reboot


BLEHandler bleHandler; 
unsigned long lastBLECheck = 0;  // Track last BLE check time

void setup() {

    // --- Get and store the reset reason ---
    esp_reset_reason_t reason = esp_reset_reason();
    Debug::setResetReason(reason);

    Serial.begin(115200);
    delay(1000); // Give some time for serial connection but no loop as we are not using the serial port in the actual meters

    LOG_TI(TAG, "\n\n--- Srcful ZAP Firmware Booting ---");

    pinMode(LED_PIN, OUTPUT);
    pinMode(IO_BUTTON, INPUT_PULLUP); // Initialize button pin with internal pull-up
    digitalWrite(LED_PIN, LOW); // Quick blink on startup
    delay(500);
    digitalWrite(LED_PIN, HIGH);


    LOG_TI(TAG, "Starting setup...");

    LOG_TI(TAG, "Total heap: %d\n", ESP.getHeapSize());
    LOG_TI(TAG, "Free heap: %d\n", ESP.getFreeHeap());
    LOG_TI(TAG, "Total PSRAM: %d\n", ESP.getPsramSize());
    LOG_TI(TAG, "Free PSRAM: %d\n", ESP.getFreePsram());

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
            LOG_TI(TAG, "No private key found in Preferences");
            uint8_t privateKey[32];
            if (crypto_create_private_key(privateKey)) {
                // convert to hex string
                char* privateKeyHex = new char [sizeof(privateKey) * 2 + 1];
                bytes_to_hex_string(privateKey, sizeof(privateKey), privateKeyHex);
                PRIVATE_KEY_HEX = privateKeyHex;
                
                // Store the private key in Preferences
                preferences.putBytes("private_key", privateKey, sizeof(privateKey));
            } else {
                LOG_TE(TAG, "Failed to create private key!!");
            }
        }

        preferences.end();
        Serial.printf("serial number: %s\n", crypto_getId().c_str());
        Serial.printf("Public key: %s\n", crypto_get_public_key(PRIVATE_KEY_HEX).c_str());
    }
    

    // Try to connect using saved credentials first.
    // if there is no saved credentials we need to start the BLE
    if (!wifiManager.loadCredentials()) {
        LOG_TI(TAG, "No saved credentials found, starting BLE setup...");
    
        bleHandler.init();
    } else {
        LOG_TI(TAG, "Found saved credentials, attempting to connect...");
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
    g_dataReaderTask.setInterval(10000); // 10 seconds interval for generating data
    g_dataReaderTask.begin(backendApiTask.getQueueHandle()); // Share the queue between tasks
    
    // Start the backend API task
    backendApiTask.begin(&wifiManager);  // Pass the WiFi manager reference
    backendApiTask.setInterval(300000);  // 5 minutes interval (300,000 ms) for state updates
    backendApiTask.setBleActive(true);   // Initialize with BLE active (same as dataSenderTask)
    
    // Initialize and start MQTT client if configured
    if (strlen(MQTT_SERVER) > 0) {
        LOG_TI(TAG, "Initializing MQTT client...");
        LOG_TI(TAG, "Free heap before MQTT init: %d", ESP.getFreeHeap());
        
        mqttClient.setWifiManager(&wifiManager);
        mqttClient.setServer(MQTT_SERVER, MQTT_PORT, MQTT_USE_SSL);
        
        // Set dynamic MQTT credentials based on device ID
        String deviceId = crypto_getId().c_str();  // This includes "zap-" prefix
        LOG_TI(TAG, "Device ID: %s", deviceId.c_str());
        LOG_TI(TAG, "Free heap before JWT generation: %d", ESP.getFreeHeap());
        
        String authJWT = generateAuthJWT(deviceId);
        LOG_TI(TAG, "Free heap after JWT generation: %d", ESP.getFreeHeap());
        
        if (authJWT.length() == 0) {
            LOG_TE(TAG, "Failed to generate auth JWT, MQTT will not work properly");
            return;
        }
        
        LOG_TI(TAG, "Setting MQTT username to device ID: %s", deviceId.c_str());
        mqttClient.setCredentials(deviceId.c_str(), authJWT.c_str());
        
        // Set client ID and subscribe topic based on device serial number
        String clientId = deviceId;
        String subscribeTopic = clientId + "/commands";
        
        LOG_TI(TAG, "Setting MQTT client ID to: %s", clientId.c_str());
        mqttClient.setClientId(clientId.c_str());
        
        LOG_TI(TAG, "Setting MQTT subscribe topic to: %s", subscribeTopic.c_str());
        mqttClient.subscribe(subscribeTopic.c_str());
        
        mqttClient.begin(&wifiManager);
        LOG_TI(TAG, "MQTT client initialized");
    } else {
        LOG_TI(TAG, "MQTT disabled (no server configured)");
    }
    
    // Start the server task
    // server task is started/restarted in the main loop when wifi is connected
    // serverTask.begin();


    LOG_TI(TAG, "Setup completed successfully!");
    LOG_TI(TAG, "Free heap after setup: %i", ESP.getFreeHeap());
}

void loop() {
    const unsigned long currentTime = millis();
    // Track WiFi connection state for MQTT status updates
    static bool wasWiFiConnected = false;
    bool isWiFiConnected = wifiManager.isConnected();
    
    // --- Check for deferred actions FIRST ---
    mainActionManager.checkAndExecute(currentTime, wifiManager, backendApiTask, bleHandler); // Call method on the instance

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
            unsigned long pressDuration = currentTime - buttonPressStartTime;
            
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
        unsigned long pressDuration = currentTime - buttonPressStartTime;
        buttonPressed = false;
        
        if (pressDuration > CLEAR_WIFI_PRESS_DURATION) {
            // Long press confirmed and button released, now reset WiFi and restart
            LOG_TD(TAG, "Long press confirmed! Resetting WiFi settings...");
            
            // Clear WiFi credentials
            wifiManager.clearCredentials();
        }

        // no else here as we want to reboot even if the button was pressed for less than 5 seconds
        // i.e. this is a reboot without wifi credentials clearing.
        if (pressDuration > REBOOT_PRESS_DURATION) {
            // Short press confirmed, trigger a reboot
            LOG_TD(TAG, "Short press confirmed! Rebooting...");
            
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

    if (wifiManager.isConnected()) {
        
        // Log MQTT status on WiFi connection
        if (!wasWiFiConnected && mqttClient.isConnected()) {
            LOG_TD(TAG, "WiFi and MQTT connected");
        }
        
        // Check if the server task is running, restart if needed
        if (!serverTask.isRunning()) {
            LOG_TD(TAG, "Server task not running, restarting...");
            serverTask.begin();
        }

        if (bleHandler.isActive()) {
            LOG_TV(TAG, "BLE is active, scheduling stop");
            MainActions::triggerAction(MainActions::Type::BLE_DISCONNECT, 30 * 1000); // Request BLE disconnect in 30 seconds
        }
        
        // Example: Publish harvest data via MQTT every 60 seconds
        static unsigned long lastMqttPublish = 0;
        if (mqttClient.isConnected() && (currentTime - lastMqttPublish > 60000)) {
            lastMqttPublish = currentTime;
            
            String serialNumber = crypto_getId().c_str();
            LOG_TI(TAG, "=== MQTT PUBLISH DEBUG ===");
            LOG_TI(TAG, "Serial number: %s", serialNumber.c_str());
            LOG_TI(TAG, "Publishing to harvest topic: %s/harvest", serialNumber.c_str());
            
            // Example: publish harvest data (replace with actual meter readings)
            char harvestData[128];
            snprintf(harvestData, sizeof(harvestData), "{\"energy\":123.45,\"timestamp\":%lu}", millis());
            mqttClient.publishHarvestData(harvestData);
            
            LOG_TI(TAG, "Publishing to heartbeat topic: %s/heartbeat", serialNumber.c_str());
            mqttClient.publishHeartbeat();
        }
    } else {
        // WiFi disconnected
        if (wasWiFiConnected) {
            LOG_TD(TAG, "WiFi disconnected");
        }
    }
    
    // Update WiFi connection state
    wasWiFiConnected = isWiFiConnected;

    if (!buttonPressed && bleHandler.isActive()) {
        // If BLE is active and button is not pressed, turn on the LED
        digitalWrite(LED_PIN, LOW);
    }
    
    // Update BLE state in data sender task and backend API task
    backendApiTask.setBleActive(bleHandler.isActive());
    
    // TODO: move to bleHandler loop method?
    // Handle BLE request queue in the main loop
    if (currentTime - lastBLECheck > 1000) {
        lastBLECheck = currentTime;
        bleHandler.handlePendingRequest();
    }

    if (bleHandler.shouldHardStop(2000)) {
        bleHandler.hardStop();
    }
    
    yield();
}

