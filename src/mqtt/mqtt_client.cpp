#include "mqtt_client.h"
#include "../config.h"
#include "../zap_log.h"

// Define TAG for logging
static const char* TAG = "mqtt_client";

// Static instance for callback
ZapMQTTClient* ZapMQTTClient::instance = nullptr;

ZapMQTTClient::ZapMQTTClient(uint32_t stackSize, UBaseType_t priority) 
    : taskHandle(nullptr), stackSize(stackSize), priority(priority), shouldRun(false),
      wifiManager(nullptr), port(1883), useSSL(false), wasConnected(false),
      lastReconnectAttempt(0), lastKeepAlive(0), mqttClient(wifiClient) {
    
    // Initialize strings
    strcpy(server, "");
    strcpy(username, "");
    strcpy(password, "");
    strcpy(clientId, "zap_client");
    strcpy(subscribeTopic, "");
    
    // Set static instance for callback
    instance = this;
}

ZapMQTTClient::~ZapMQTTClient() {
    stop();
    instance = nullptr;
}

void ZapMQTTClient::begin(WifiManager* wifiManager) {
    if (taskHandle != nullptr) {
        return; // Task already running
    }
    
    this->wifiManager = wifiManager;
    shouldRun = true;
    
    // Configure MQTT client
    if (useSSL) {
        LOG_I(TAG, "Configuring SSL connection with CA certificate");
        
        // Try with CA certificate first
        wifiClientSecure.setCACert(MQTT_CA_CERT);
        
        // Add some SSL debugging
        wifiClientSecure.setHandshakeTimeout(30000); // 30 second timeout
        
        mqttClient.setClient(wifiClientSecure);
        LOG_I(TAG, "SSL client configured with CA certificate");
    } else {
        LOG_I(TAG, "Configuring non-SSL connection");
        mqttClient.setClient(wifiClient);
    }
    
    mqttClient.setServer(server, port);
    mqttClient.setCallback(mqttCallback);
    mqttClient.setKeepAlive(60); // 60 seconds keep-alive
    mqttClient.setSocketTimeout(15); // 15 seconds timeout
    
    // Increase buffer size for larger JSON messages (default is 256 bytes)
    mqttClient.setBufferSize(2048); // Set to 2KB to handle larger JSON messages
    LOG_I(TAG, "MQTT buffer size set to 2048 bytes");
    
    // Create the task
    BaseType_t result = xTaskCreatePinnedToCore(
        taskFunction,
        "ZapMQTTClient",
        stackSize,
        this,
        priority,
        &taskHandle,
        0  // Run on core 0
    );
    
    if (result != pdPASS) {
        LOG_E(TAG, "Failed to create MQTT task!");
        shouldRun = false;
    } else {
        LOG_I(TAG, "MQTT task created successfully");
    }
}

void ZapMQTTClient::stop() {
    if (taskHandle == nullptr) {
        return; // Task not running
    }
    
    shouldRun = false;
    
    // Disconnect MQTT if connected
    if (mqttClient.connected()) {
        mqttClient.disconnect();
    }
    
    vTaskDelay(pdMS_TO_TICKS(100)); // Give the task time to exit
    
    if (taskHandle != nullptr) {
        vTaskDelete(taskHandle);
        taskHandle = nullptr;
    }
    
    LOG_I(TAG, "MQTT task stopped");
}

void ZapMQTTClient::setServer(const char* server, uint16_t port, bool useSSL) {
    strncpy(this->server, server, sizeof(this->server) - 1);
    this->server[sizeof(this->server) - 1] = '\0';
    this->port = port;
    this->useSSL = useSSL;
    
    LOG_I(TAG, "MQTT server set to %s:%d (SSL: %s)", server, port, useSSL ? "yes" : "no");
}

void ZapMQTTClient::setCredentials(const char* username, const char* password) {
    strncpy(this->username, username, sizeof(this->username) - 1);
    this->username[sizeof(this->username) - 1] = '\0';
    strncpy(this->password, password, sizeof(this->password) - 1);
    this->password[sizeof(this->password) - 1] = '\0';
    
    LOG_I(TAG, "MQTT credentials set for user: %s", username);
    LOG_I(TAG, "Password length: %d characters", strlen(password));
    
    // Check if password was truncated
    if (strlen(password) >= sizeof(this->password)) {
        LOG_E(TAG, "WARNING: Password truncated! Original: %d, Buffer: %d", 
              strlen(password), sizeof(this->password));
    }
}

void ZapMQTTClient::setClientId(const char* clientId) {
    strncpy(this->clientId, clientId, sizeof(this->clientId) - 1);
    this->clientId[sizeof(this->clientId) - 1] = '\0';
    
    LOG_I(TAG, "MQTT client ID set to: %s", clientId);
}

void ZapMQTTClient::subscribe(const char* topic) {
    strncpy(this->subscribeTopic, topic, sizeof(this->subscribeTopic) - 1);
    this->subscribeTopic[sizeof(this->subscribeTopic) - 1] = '\0';
    
    LOG_I(TAG, "MQTT subscribe topic set to: %s", topic);
    
    // If already connected, subscribe immediately
    if (mqttClient.connected() && strlen(topic) > 0) {
        if (mqttClient.subscribe(topic)) {
            LOG_I(TAG, "Successfully subscribed to: %s", topic);
        } else {
            LOG_E(TAG, "Failed to subscribe to: %s", topic);
        }
    }
}

void ZapMQTTClient::publish(const char* topic, const char* payload, bool retained) {
    if (mqttClient.connected()) {
        if (mqttClient.publish(topic, payload, retained)) {
            LOG_D(TAG, "Published to %s: %s", topic, payload);
        } else {
            LOG_E(TAG, "Failed to publish to: %s", topic);
        }
    } else {
        LOG_W(TAG, "Cannot publish - MQTT not connected");
    }
}

void ZapMQTTClient::publishHarvestData(const char* harvestData) {
    char topic[64];
    snprintf(topic, sizeof(topic), "%s/harvest", clientId);
    publish(topic, harvestData, false);
}

void ZapMQTTClient::publishHeartbeat() {
    char topic[64];
    char payload[128];
    snprintf(topic, sizeof(topic), "%s/heartbeat", clientId);
    snprintf(payload, sizeof(payload), "{\"uptime\":%lu,\"heap\":%d}", millis(), ESP.getFreeHeap());
    publish(topic, payload, false);
}

bool ZapMQTTClient::connectToMQTT() {
    if (!wifiManager || !wifiManager->isConnected()) {
        LOG_W(TAG, "WiFi not connected, skipping MQTT connection");
        return false;
    }
    
    LOG_I(TAG, "=== ATTEMPTING MQTT CONNECTION ===");
    LOG_I(TAG, "Broker: %s:%d (SSL: %s)", server, port, useSSL ? "yes" : "no");
    LOG_I(TAG, "Client ID: '%s'", clientId);
    LOG_I(TAG, "Will subscribe to: '%s'", subscribeTopic);
    
    // Configure WiFi client for SSL if needed
    if (useSSL) {
        wifiClientSecure.setCACert(MQTT_CA_CERT);
        // Use secure client
        if (strlen(username) > 0) {
            if (mqttClient.connect(clientId, username, password)) {
                LOG_I(TAG, "MQTT connected with SSL and credentials");
                // Subscribe to topic if set
                if (strlen(subscribeTopic) > 0) {
                    if (mqttClient.subscribe(subscribeTopic)) {
                        LOG_I(TAG, "Successfully subscribed to: %s", subscribeTopic);
                        return true;
                    } else {
                        LOG_E(TAG, "Failed to subscribe to: %s", subscribeTopic);
                        return false;
                    }
                }
                return true;
            }
        } else {
            if (mqttClient.connect(clientId)) {
                LOG_I(TAG, "MQTT connected with SSL");
                // Subscribe to topic if set
                if (strlen(subscribeTopic) > 0) {
                    if (mqttClient.subscribe(subscribeTopic)) {
                        LOG_I(TAG, "Successfully subscribed to: %s", subscribeTopic);
                        return true;
                    } else {
                        LOG_E(TAG, "Failed to subscribe to: %s", subscribeTopic);
                        return false;
                    }
                }
                return true;
            }
        }
    } else {
        // Use regular client
        if (strlen(username) > 0) {
            if (mqttClient.connect(clientId, username, password)) {
                LOG_I(TAG, "MQTT connected with credentials");
                // Subscribe to topic if set
                if (strlen(subscribeTopic) > 0) {
                    if (mqttClient.subscribe(subscribeTopic)) {
                        LOG_I(TAG, "Successfully subscribed to: %s", subscribeTopic);
                        return true;
                    } else {
                        LOG_E(TAG, "Failed to subscribe to: %s", subscribeTopic);
                        return false;
                    }
                }
                return true;
            }
        } else {
            if (mqttClient.connect(clientId)) {
                LOG_I(TAG, "MQTT connected");
                // Subscribe to topic if set
                if (strlen(subscribeTopic) > 0) {
                    if (mqttClient.subscribe(subscribeTopic)) {
                        LOG_I(TAG, "Successfully subscribed to: %s", subscribeTopic);
                        return true;
                    } else {
                        LOG_E(TAG, "Failed to subscribe to: %s", subscribeTopic);
                        return false;
                    }
                }
                return true;
            }
        }
    }
    
    LOG_E(TAG, "MQTT connection failed! State: %d", mqttClient.state());
    return false;
}

void ZapMQTTClient::reconnect() {
    unsigned long now = millis();
    
    // Try to reconnect every 5 seconds
    if (now - lastReconnectAttempt > 5000) {
        lastReconnectAttempt = now;
        
        if (connectToMQTT()) {
            lastReconnectAttempt = 0;
        }
    }
}

void ZapMQTTClient::taskFunction(void* parameter) {
    ZapMQTTClient* client = static_cast<ZapMQTTClient*>(parameter);
    
    // Sleep for a bit to allow other tasks to initialize
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    LOG_I(TAG, "MQTT client task started");
    
    while (client->shouldRun) {
        // Only operate when WiFi is connected
        if (client->wifiManager && client->wifiManager->isConnected()) {
            
            if (!client->mqttClient.connected()) {
                // Lost connection
                if (client->wasConnected) {
                    LOG_W(TAG, "MQTT connection lost");
                    client->wasConnected = false;
                }
                
                // Try to reconnect
                client->reconnect();
            } else {
                // Connected - handle MQTT loop
                client->mqttClient.loop();
                
                // Send periodic keep-alive or heartbeat if needed
                unsigned long now = millis();
                if (now - client->lastKeepAlive > 30000) { // Every 30 seconds
                    client->lastKeepAlive = now;
                    // Send heartbeat with device status
                    client->publishHeartbeat();
                }
            }
            
        } else {
            // WiFi not connected
            if (client->mqttClient.connected()) {
                LOG_I(TAG, "WiFi disconnected, closing MQTT connection");
                client->mqttClient.disconnect();
                client->wasConnected = false;
            }
        }
        
        // Small delay to prevent task from hogging CPU
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    // Task cleanup
    if (client->mqttClient.connected()) {
        client->mqttClient.disconnect();
    }
    
    LOG_I(TAG, "MQTT client task ended");
    vTaskDelete(NULL);
}

void ZapMQTTClient::mqttCallback(char* topic, byte* payload, unsigned int length) {
    if (instance == nullptr) return;
    
    // Convert payload to string
    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';
    
    // Print received message to console
    Serial.printf("MQTT: %s\n", message);
    
    // Simple command handling - check if this is our commands topic
    if (strcmp(topic, instance->subscribeTopic) == 0) {
        if (strcmp(message, "heartbeat") == 0) {
            // Send immediate heartbeat
            instance->publishHeartbeat();
        } else {
            // Handle other JSON commands here
            // To extend: add more else-if conditions for new commands
        }
    }
}
