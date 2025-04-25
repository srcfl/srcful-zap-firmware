#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <time.h>

#include "json_light/json_light.h"

// Forward declarations
class Crypto;
class BlackBoard;

/**
 * A minimal WebSocket client implementation for ESP32
 * Designed specifically for GraphQL subscriptions
 */
class GraphQLSubscriptionClient {
private:
    // Constants
    const unsigned long PING_INTERVAL = 45000; // 45 seconds in milliseconds
    const unsigned long RECONNECT_DELAY = 5000; // 5 seconds in milliseconds
    const char* SETTINGS_SUBKEY = "settings";
    const char* REQUEST_TASK_SUBKEY = "request";

    // Instance properties
    WiFiClientSecure client; // Use WiFiClientSecure for WSS
    String host;
    uint16_t port;
    String path;
    String url;
    bool _isConnected = false;
    bool isWebSocketHandshakeDone = false;
    unsigned long lastPingTime = 0;
    unsigned long lastConnectAttempt = 0;


public:

    GraphQLSubscriptionClient(const char* wsUrl);
    
    // Initialize connection
    bool begin();
    
    // Must be called in the main loop
    void loop();
    
    // Restart connection
    void restart();
    
    // Close connection
    void stop();
    
    // Send connection initialization
    void sendConnectionInit();
    
    // Subscribe to settings
    void subscribeToSettings();
    
    // Generate subscription query with authentication
    zap::Str getSubscriptionQuery();

    bool isConnected() const {
        return _isConnected;
    }
    
    
private:
    // Parse URL into components
    void parseUrl(String url);
    
    // Perform WebSocket handshake
    bool performWebSocketHandshake();
    
    // Send ping to keep connection alive
    void sendPing();
    
    // Send a WebSocket frame
    void sendFrame(const zap::Str& data, uint8_t opcode = 0x01); // 0x01 = text frame
    
    // Process received WebSocket data
    void processWebSocketData(uint8_t* data, size_t length);
    
    // Helper for handling settings data
    void handleSettings(JsonParser& configData);
    
    // Helper for handling request tasks
    void handleRequestTask(JsonParser& configData);


    void processConfiguration(const char* configData);
    
    // Request handling methods
    void handleRequest(JsonParser& configData);
    void sendResponse(const zap::Str& requestId, int statusCode, const zap::Str& responseData);
    void sendErrorResponse(const zap::Str& requestId, const char* errorMessage);
};

