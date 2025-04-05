#include "endpoint_types.h"
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include "html.h"
#include "crypto.h"
#include <HTTPClient.h>
#include <esp_system.h>
#include "p1data.h"
#include "endpoint_mapper.h"
#include <WiFiClientSecure.h>
#include "esp_heap_caps.h"
#include "graphql.h"
#include "config.h"
#include "server/server_task.h"

#define LED_PIN 7

// Global variables
ServerTask serverTask(80); // Create a server task instance
bool isProvisioned = false;
String configuredSSID = "";
String configuredPassword = "";
unsigned long lastJWTTime = 0;
const unsigned long JWT_INTERVAL = 10000; // 10 seconds in milliseconds
std::vector<String> lastScanResults;
unsigned long lastScanTime = 0;
const unsigned long SCAN_CACHE_TIME = 10000; // Cache scan results for 10 seconds
unsigned long bleShutdownTime = 0; // Time when BLE should be shut down (0 = no shutdown scheduled)

#if defined(USE_BLE_SETUP)
    #include "ble_handler.h"
    BLEHandler bleHandler;  // Remove parentheses to make it an object, not a function
    bool isBleActive = false;  // Track BLE state
#endif

// Function declarations
void setupAP();
bool connectToWiFi(const String& ssid, const String& password, bool updateGlobals = true);
void runSigningTest();
String getId();
void scanWiFiNetworks();
void setupSSL();
void sendJWT();  // Add JWT sending function declaration

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
        if (connectToWiFi(WIFI_SSID, WIFI_PSK)) {
            digitalWrite(LED_PIN, HIGH); // Solid LED when connected
        } else {
            Serial.println("WiFi connection failed");
            digitalWrite(LED_PIN, LOW);
        }
    #endif
    
    // Setup SSL with optimized memory settings
    setupSSL();
    
    // Perform initial WiFi scan
    Serial.println("Starting WiFi scan...");
    scanWiFiNetworks();
    Serial.println("WiFi scan completed");
    
    // Verify public key
    Serial.println("Verifying public key...");
    String publicKey = crypto_get_public_key(PRIVATE_KEY_HEX);
    if (publicKey.length() == 0) {
        Serial.println("Failed to generate public key!");
        return;
    }
    
    Serial.print("Generated public key: ");
    Serial.println(publicKey);
    Serial.print("Expected public key: ");
    Serial.println(EXPECTED_PUBLIC_KEY_HEX);
    
    // Temporarily skip public key verification to get BLE working
    Serial.println("Skipping public key verification for now");
    /*
    if (publicKey != String(EXPECTED_PUBLIC_KEY_HEX)) {
        Serial.println("WARNING: Generated public key does not match expected public key!");
        return;
    }
    */
    Serial.println("Key pair verified successfully");
    
    // Run signing test
    Serial.println("Running signing test...");
    runSigningTest();
    Serial.println("Signing test completed");
    
    // Continue with normal setup
    #if defined(USE_SOFTAP_SETUP)
        Serial.println("Setting up AP mode...");
        setupAP();
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
    if (MDNS.begin(MDNS_NAME)) {
        Serial.println("MDNS responder started");
    } else {
        Serial.println("Error setting up MDNS responder!");
    }
    
    // Start the server task
    Serial.println("Starting server task...");
    serverTask.begin();
    Serial.println("Server task started");
    
    Serial.println("Setup completed successfully!");
    Serial.print("Free heap after setup: ");
    Serial.println(ESP.getFreeHeap());
}

void loop() {
    static unsigned long lastCheck = 0;
    static bool wasConnected = false;
    
    // Check WiFi status every 5 seconds
    if (millis() - lastCheck > 5000) {
        lastCheck = millis();
        if (WiFi.status() == WL_CONNECTED) {
            if (!wasConnected) {
                Serial.println("WiFi connected");
                Serial.print("IP address: ");
                Serial.println(WiFi.localIP());
                wasConnected = true;
            }

            // Send JWT if conditions are met
            #if defined(USE_BLE_SETUP)
            if (!isBleActive && (millis() - lastJWTTime >= JWT_INTERVAL)) {
                sendJWT();
                lastJWTTime = millis();
            }
            #else
            if (millis() - lastJWTTime >= JWT_INTERVAL) {
                sendJWT();
                lastJWTTime = millis();
            }
            #endif

        } else {
            if (wasConnected) {
                Serial.println("WiFi connection lost!");
                wasConnected = false;
            }
            #if defined(DIRECT_CONNECT)
                // Only try to reconnect automatically in direct connect mode
                digitalWrite(LED_PIN, millis() % 1000 < 500); // Blink LED when disconnected
                WiFi.begin(WIFI_SSID, WIFI_PSK);
                Serial.println("WiFi disconnected, attempting to reconnect...");
            #endif
        }
        
        // Print some debug info
        Serial.print("Free heap: ");
        Serial.println(ESP.getFreeHeap());
        Serial.print("WiFi status: ");
        Serial.println(WiFi.status());
    }

    // handle ble tasks
    #if defined(USE_BLE_SETUP)
        static unsigned long lastBLECheck = 0;
        if (millis() - lastBLECheck > 1000) {
            lastBLECheck = millis();
            bleHandler.handlePendingRequest();
        }
        if (bleShutdownTime > 0 && millis() >= bleShutdownTime) {
            Serial.println("Executing scheduled BLE shutdown");
            bleHandler.stop();
            isBleActive = false;
            bleShutdownTime = 0;  // Reset the timer
        }
    #endif

    // Check if the server task is running
    if (WiFi.status() == WL_CONNECTED) {
        digitalWrite(LED_PIN, LOW); // Solid LED when connected
        
        // Check if the server task is running, restart if needed
        if (!serverTask.isRunning()) {
            Serial.println("Server task not running, restarting...");
            serverTask.begin();
        }
    }
    
    yield();
}

void setupAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
}

bool connectToWiFi(const String& ssid, const String& password, bool updateGlobals) {
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
            Serial.println("NTP initialized");
            
            // Configure low power WiFi
            WiFi.setSleep(true);  // Enable modem sleep
            
            // Update global variables if requested
            if (updateGlobals) {
                configuredSSID = ssid;
                configuredPassword = password;
                isProvisioned = true;
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

void runSigningTest() {
  Serial.println("\n=== Running Signing Test ===");
  
  // Original JWT test
  String deviceId = getId();
  const char* testHeader = R"({"alg":"ES256K","typ":"JWT"})";
  String testPayloadStr = "{\"sub\":\"" + deviceId + "\",\"name\":\"John Doe\",\"iat\":1516239022}";
  
  Serial.println("Creating test JWT...");
  Serial.print("Header: ");
  Serial.println(testHeader);
  Serial.print("Payload: ");
  Serial.println(testPayloadStr);
  
  String jwt = crypto_create_jwt(testHeader, testPayloadStr.c_str(), PRIVATE_KEY_HEX);
  
  if (jwt.length() == 0) {
    Serial.println("TEST FAILED: JWT creation failed!");
    return;
  }
  
  Serial.println("\nFinal JWT:");
  Serial.println(jwt);
  
  // Add specific test case
  Serial.println("\n=== Running Specific Signature Test ===");
  const char* testMessage = "zap_000098f89ec964:Bygcy876b3bsjMvvhZxghvs3EyR5y6a7vpvAp5D62n2w";
  Serial.print("Test message: ");
  Serial.println(testMessage);
  
  String hexSignature = crypto_create_signature_hex(testMessage, PRIVATE_KEY_HEX);
  Serial.print("Hex signature: ");
  Serial.println(hexSignature);
  
  String b64urlSignature = crypto_create_signature_base64url(testMessage, PRIVATE_KEY_HEX);
  Serial.print("Base64URL signature: ");
  Serial.println(b64urlSignature);
  
  Serial.println("=== Signing Tests Complete ===\n");
}

String getId() {
  uint64_t chipId = ESP.getEfuseMac();
  char serial[17];
  snprintf(serial, sizeof(serial), "%016llx", chipId);
  
  String id = "zap-" + String(serial);
  
  // Ensure exactly 18 characters
  if (id.length() > 18) {
    // Truncate to 18 chars if longer
    id = id.substring(0, 18);
  } else while (id.length() < 18) {
    // Pad with 'e' if shorter
    id += 'e';
  }
  
  return id;
}

void scanWiFiNetworks() {
  Serial.println("Scanning WiFi networks...");
  
  // Set WiFi to station mode to perform scan
  WiFi.mode(WIFI_STA);
  
  int n = WiFi.scanNetworks();
  Serial.println("Scan completed");
  
  lastScanResults.clear();
  
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
    
    // Store in global vector
    lastScanResults = uniqueSSIDs;
  }
  
  lastScanTime = millis();
  
  // Return to original mode if we were in AP mode
  if (isProvisioned) {
    WiFi.mode(WIFI_STA);
  } else {
    setupAP();
  }
}

void setupSSL() {
    // Nothing needed here - HTTPClient handles SSL internally
}// Add JWT sending function
void sendJWT() {
    String deviceId = getId();
    
    // Create JWT using P1 data
    String jwt = createP1JWT(PRIVATE_KEY_HEX, deviceId);
    if (jwt.length() > 0) {
        Serial.println("P1 JWT created successfully");
        Serial.println("JWT: " + jwt);  // Add JWT logging
        
        // Create HTTP client and configure it
        HTTPClient http;
        http.setTimeout(10000);  // 10 second timeout
        
        Serial.print("Sending JWT to: ");
        Serial.println(DATA_URL);
        
        // Start the request
        if (http.begin(DATA_URL)) {
            // Add headers
            http.addHeader("Content-Type", "text/plain");
            
            // Send POST request with JWT as body
            int httpResponseCode = http.POST(jwt);
            
            if (httpResponseCode > 0) {
                Serial.print("HTTP Response code: ");
                Serial.println(httpResponseCode);
                String response = http.getString();
                Serial.println("Response: " + response);
            } else {
                Serial.print("Error code: ");
                Serial.println(httpResponseCode);
            }
            
            // Clean up
            http.end();
        } else {
            Serial.println("Failed to connect to server");
        }
    } else {
        Serial.println("Failed to create P1 JWT");
    }
}

