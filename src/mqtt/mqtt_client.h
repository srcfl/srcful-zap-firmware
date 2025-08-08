#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <WiFiClientSecure.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include "../wifi/wifi_manager.h"
#include "../zap_str.h"

class ZapMQTTClient {
public:
    explicit ZapMQTTClient(uint32_t stackSize = 1024 * 8, UBaseType_t priority = 5);
    ~ZapMQTTClient();
    
    // Initialize and start the task
    void begin(WifiManager* wifiManager);
    
    // Stop the task
    void stop();
    
    // Task status
    bool isRunning() const { return taskHandle != nullptr; }
    bool isConnected() { return mqttClient.connected(); }
    
    // Configuration
    void setServer(const char* server, uint16_t port, bool useSSL = false);
    void setCredentials(const char* username, const char* password);
    void setClientId(const char* clientId);
    void subscribe(const char* topic);
    void publish(const char* topic, const char* payload, bool retained = false);
    
    // Convenience methods for common operations
    void publishHarvestData(const char* harvestData);
    void publishCommandAck(const char* decisionId, const char* status, const char* message);
    
    // Set the WiFi manager instance
    void setWifiManager(WifiManager* manager) { wifiManager = manager; }
    
private:
    // The actual task function
    static void taskFunction(void* parameter);
    
    // MQTT callback for received messages
    static void mqttCallback(char* topic, byte* payload, unsigned int length);
    
    // Command processing
    void processCommand(const char* commandJson);
    
    // Connection management
    bool connectToMQTT();
    void reconnect();
    
    // Task handle and parameters
    TaskHandle_t taskHandle;
    uint32_t stackSize;
    UBaseType_t priority;
    bool shouldRun;
    
    // WiFi manager instance
    WifiManager* wifiManager;
    
    // MQTT configuration
    char server[128];
    uint16_t port;
    bool useSSL;
    char username[64];
    char password[512];  // Increased for long JWTs
    char clientId[64];
    char subscribeTopic[128];
    
    // MQTT client instances
    WiFiClient wifiClient;
    WiFiClientSecure wifiClientSecure;
    PubSubClient mqttClient;
    
    // Connection state
    bool wasConnected;
    unsigned long lastReconnectAttempt;
    unsigned long lastKeepAlive;
    
    // Static instance for callback
    static ZapMQTTClient* instance;
};

#endif // MQTT_CLIENT_H
