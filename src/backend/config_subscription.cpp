#include "config_subscription.h"

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <time.h>

#include "crypto.h"
#include "config.h"

#include "endpoints/endpoint_mapper.h"
#include "endpoints/endpoint_types.h"
#include "endpoints/endpoints.h"
#include "backend/request_handler.h" // Ensure RequestHandler is included
#include "backend/graphql.h" // Include for GQL namespace used in removed methods


class RequestHandlerExternals : public zap::backend::RequestHandler::Externals {
public:
    GQL::BoolResponse setConfiguration(const zap::Str& jwt) override {
        return GQL::setConfiguration(jwt);
    }

    const Endpoint& toEndpoint(const zap::Str& path, const zap::Str& verb) override {
        return EndpointMapper::toEndpoint(path, verb);
    }

    EndpointResponse route(const EndpointRequest& request) override {
        return EndpointMapper::route(request);
    }
};

RequestHandlerExternals g_requestHandlerExternals;

// Constructor
GraphQLSubscriptionClient::GraphQLSubscriptionClient(const char* wsUrl) : requestHandler(g_requestHandlerExternals) {
    url = String(wsUrl);
    parseUrl(url);
}

// Parse URL into components
void GraphQLSubscriptionClient::parseUrl(String urlString) {
    // Remove protocol
    int protocolEnd = urlString.indexOf("://");
    if (protocolEnd > 0) {
        urlString = urlString.substring(protocolEnd + 3);
    }
    
    // Extract host, port, and path
    int portStart = urlString.indexOf(':');
    int pathStart = urlString.indexOf('/');
    
    if (pathStart == -1) {
        pathStart = urlString.length();
        path = "/";
    } else {
        path = urlString.substring(pathStart);
    }
    
    if (portStart == -1) {
        port = 443; // Default HTTP port
        host = urlString.substring(0, pathStart);
    } else {
        host = urlString.substring(0, portStart);
        port = urlString.substring(portStart + 1, pathStart).toInt();
    }
}

// Initialize the client
bool GraphQLSubscriptionClient::begin() {
    Serial.println("Connecting to WebSocket server...");

    client.setInsecure(); // Accept all SSL certificates (not recommended for production)
    client.setTimeout(10);
    
    if (client.connect(host.c_str(), port)) {
        Serial.println("TCP Connection established");
        _isConnected = true;
        
        // Perform WebSocket handshake
        if (performWebSocketHandshake()) {
            Serial.println("WebSocket handshake successful");
            isWebSocketHandshakeDone = true;
            sendConnectionInit();
            return true;
        } 
        else {
            Serial.println("WebSocket handshake failed");
            client.stop();
            _isConnected = false;
            return false;
        }
    } 
    else {
        Serial.println("TCP Connection failed");
        return false;
    }
}

// Perform WebSocket handshake
bool GraphQLSubscriptionClient::performWebSocketHandshake() {
    // Generate random key for the handshake
    String key = "";
    for (int i = 0; i < 16; i++) {
        key += (char)random(0, 255);
    }
    
    // Base64 encode the key
    String encodedKey = "";
    const char* base64chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int val = 0, valb = -6;
    for (int i = 0; i < key.length(); i++) {
        unsigned char c = key.charAt(i);
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            encodedKey += base64chars[(val >> valb) & 0x3F];
            valb -= 6;
        }
    }
    if (valb > -6) {
        encodedKey += base64chars[((val << 8) >> (valb + 8)) & 0x3F];
    }
    while (encodedKey.length() % 4) {
        encodedKey += "=";
    }
    
    // Send WebSocket upgrade request
    client.print("GET " + path + " HTTP/1.1\r\n");
    client.print("Host: " + host + "\r\n");
    client.print("Upgrade: websocket\r\n");
    client.print("Connection: Upgrade\r\n");
    client.print("Sec-WebSocket-Key: " + encodedKey + "\r\n");
    client.print("Sec-WebSocket-Version: 13\r\n");
    client.print("Sec-WebSocket-Protocol: graphql-ws\r\n");
    client.print("\r\n");
    
    // Wait for server response
    unsigned long timeout = millis() + 5000;
    while (client.available() == 0) {
        if (millis() > timeout) {
            Serial.println("Handshake timeout");
            return false;
        }
        delay(10);
    }
    
    // Read server response
    String response = "";
    while (client.available()) {
        char c = client.read();
        response += c;
        
        // Check for end of headers
        if (response.endsWith("\r\n\r\n")) {
            break;
        }
    }
    
    // Check if upgrade was successful
    if (response.indexOf("HTTP/1.1 101") < 0) {
        Serial.println("Handshake failed, server response:");
        Serial.println(response);
        return false;
    }
    
    return true;
}

// Main loop function to handle WebSocket events
void GraphQLSubscriptionClient::loop() {
    if (!_isConnected) {
        unsigned long currentMillis = millis();
        if (currentMillis - lastConnectAttempt > RECONNECT_DELAY) {
            lastConnectAttempt = currentMillis;
            begin();
        }
        return;
    }
    
    // Check if we need to send a ping
    unsigned long currentMillis = millis();
    if (currentMillis - lastPingTime > PING_INTERVAL) {
        Serial.println("Websocket Sending ping");
        sendPing();
        lastPingTime = currentMillis;
    }
    
    // Check for available data
    if (client.available()) {
        Serial.println("Web socket Data available");
        uint8_t buffer[1024]; // Adjust buffer size as needed
        size_t length = client.read(buffer, sizeof(buffer));
        
        
        if (length > 0) {
            processWebSocketData(buffer, length);
        }
    }
    
    // Check if connection is still alive
    if (!client.connected()) {
        Serial.println("Connection lost");
        _isConnected = false;
        isWebSocketHandshakeDone = false;
        client.stop();
    }
}

// Process WebSocket frames
void GraphQLSubscriptionClient::processWebSocketData(uint8_t* buffer, size_t length) {

    for (size_t i = 0; i < length; i++) {
        Serial.print(buffer[i], HEX);
        Serial.print(" ");
    }

    // Very basic WebSocket frame parsing
    if (length < 2) return;
    
    // Check if this is a final fragment and get opcode
    bool fin = (buffer[0] & 0x80) != 0;
    uint8_t opcode = buffer[0] & 0x0F;
    
    // Check if masked
    bool masked = (buffer[1] & 0x80) != 0;
    
    // Get payload length
    uint64_t payload_length = buffer[1] & 0x7F;
    size_t header_length = 2;
    
    if (payload_length == 126) {
        // Extended payload length (16 bits)
        if (length < 4) return;
        payload_length = (buffer[2] << 8) | buffer[3];
        header_length = 4;
    } else if (payload_length == 127) {
        // Extended payload length (64 bits)
        if (length < 10) return;
        payload_length = 0;
        for (int i = 0; i < 8; i++) {
            payload_length = (payload_length << 8) | buffer[2 + i];
        }
        header_length = 10;
    }
    
    // Get masking key (if masked)
    uint8_t mask[4] = {0, 0, 0, 0};
    if (masked) {
        if (length < header_length + 4) return;
        for (int i = 0; i < 4; i++) {
            mask[i] = buffer[header_length + i];
        }
        header_length += 4;
    }
    
    // Validate length
    if (length < header_length + payload_length) return;
    
    // Process based on opcode
    switch (opcode) {
        case 0x01: // Text frame
        {
            // Allocate buffer for payload
            char* payload = new char[payload_length + 1];
            
            // Unmask payload
            if (masked) {
                for (size_t i = 0; i < payload_length; i++) {
                    payload[i] = buffer[header_length + i] ^ mask[i % 4];
                }
            } else {
                for (size_t i = 0; i < payload_length; i++) {
                    payload[i] = buffer[header_length + i];
                }
            }
            
            // Null-terminate
            payload[payload_length] = '\0';
            
            // Parse JSON
            JsonParser doc(payload);
            
            zap::Str type;
            if (doc.getString("type", type)) {
            
                if (type == "connection_ack") {
                    Serial.println("Connection acknowledged, sending subscription");
                    subscribeToSettings();
                } else if (type == "data") {
                    // Serial.print("Received data:"); Serial.println(payload);
                    JsonParser configChanges("");
                    if (doc.getObjectByPath("payload.data.configurationDataChanges", configChanges)) {

                        zap::Str subKey;
                        if (configChanges.getString("subKey", subKey)) {
                        
                            if (subKey == SETTINGS_SUBKEY) {
                                handleSettings(configChanges);
                            }
                            else if (subKey == REQUEST_TASK_SUBKEY) {
                                requestHandler.handleRequestTask(configChanges);
                            }
                        }
                    }
                }
            }
            delete[] payload;
        }
        break;
        case 0x08: // Close frame
        {
            uint16_t closeCode = 0;
            if (payload_length >= 2) {
                closeCode = (buffer[header_length] << 8) | buffer[header_length + 1];
            }
            
            Serial.print("Received close frame with code: ");
            Serial.println(closeCode);
            
            if (payload_length > 2) {
                // Extract close reason if provided
                char* reason = new char[payload_length - 1];
                for (size_t i = 0; i < payload_length - 2; i++) {
                    if (masked) {
                        reason[i] = buffer[header_length + 2 + i] ^ mask[(i + 2) % 4];
                    } else {
                        reason[i] = buffer[header_length + 2 + i];
                    }
                }
                reason[payload_length - 3] = '\0';
                Serial.print("Close reason: ");
                Serial.println(reason);
                delete[] reason;
            }
            
            stop();
        }
        break;
        case 0x09: // Ping frame
        {
            // Respond with pong
            Serial.println("Received ping, sending pong");
            // Extract ping payload
            uint8_t* pingPayload = new uint8_t[payload_length];
            for (size_t i = 0; i < payload_length; i++) {
                if (masked) {
                    pingPayload[i] = buffer[header_length + i] ^ mask[i % 4];
                } else {
                    pingPayload[i] = buffer[header_length + i];
                }
            }
            
            // Send pong with same payload
            client.write(0x8A); // FIN bit set, opcode 0x0A (pong)
            client.write((uint8_t)payload_length); // Payload length
            client.write(pingPayload, payload_length); // Payload
            
            delete[] pingPayload;
        }
        break;
        case 0x0A: // Pong frame
        {
            Serial.println("Received pong");
        }
        break;
        default:
        {
            Serial.print("Received unknown frame type: ");
            Serial.println(opcode);
        }
        break;
    }
}

// Send a WebSocket frame
void GraphQLSubscriptionClient::sendFrame(const zap::Str& data, uint8_t opcode) {
    if (!_isConnected || !isWebSocketHandshakeDone) return;
    
    size_t length = data.length();
    
    // Create a random mask (required for client to server communication)
    uint8_t mask[4];
    for (int i = 0; i < 4; i++) {
        mask[i] = random(0, 256);
    }
    
    // Write FIN bit and opcode
    client.write(0x80 | opcode); // FIN bit set
    
    // Write length WITH mask bit set
    if (length < 126) {
        client.write(0x80 | (uint8_t)length); // 0x80 sets the mask bit
    } else if (length < 65536) {
        client.write(0x80 | 126);
        client.write((uint8_t)(length >> 8));
        client.write((uint8_t)(length & 0xFF));
    } else {
        client.write(0x80 | 127);
        for (int i = 7; i >= 0; i--) {
            client.write((uint8_t)((length >> (i * 8)) & 0xFF));
        }
    }
    
    // Write the mask
    client.write(mask, 4);
    
    // Write masked payload
    const char* payload = data.c_str();
    uint8_t retries = 0;
    for (size_t i = 0; i < length && retries < 5; ) {
        if (client.write(payload[i] ^ mask[i % 4]) ) {
            i++;
            retries = 0;
        } else {
            Serial.println("Failed to send data, retrying...");
            retries++;
        }
    }
}

// Send ping to keep connection alive
void GraphQLSubscriptionClient::sendPing() {
    if (_isConnected && isWebSocketHandshakeDone) {
        // Send WebSocket ping frame
        static uint8_t pingPayload[2] = {0x89, 0x00}; // Empty payload
        client.write(pingPayload, 2); // FIN bit set, opcode 0x09 (ping)
        Serial.println("Sent ping");
    }
}

// Send connection initialization message
void GraphQLSubscriptionClient::sendConnectionInit() {
    if (_isConnected && isWebSocketHandshakeDone) {       
        sendFrame("{\"type\":\"connection_init\", \"payload\": {}}");
        Serial.println("Sent connection_init message");
    }
}

zap::Str jsonEncodeString(const zap::Str& input) {
    zap::Str result;
    for (size_t i = 0; i < input.length(); i++) {
        char c = input.c_str()[i];
        switch (c) {
            case '\"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '/': result += "\\/"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default:
                if (c < 32) {
                    // Control characters as hex
                    char hex[7];
                    snprintf(hex, sizeof(hex), "\\u%04x", c);
                    result += hex;
                } else {
                    result += c;
                }
                break;
        }
    }
    return result;
}

// Subscribe to configuration changes
void GraphQLSubscriptionClient::subscribeToSettings() {
    if (_isConnected && isWebSocketHandshakeDone) {
        zap::Str query = getSubscriptionQuery();

        // Properly escape the query string for JSON
        // Replace all newlines with \n
        // remove all newlines
        query = jsonEncodeString(query);
        
        JsonBuilder doc;
        doc.beginObject()
            .add("id", "1")
            .add("type", "start")
            .beginObject("payload")
                .add("query", query.c_str())
            .endObject();

        zap::Str payload = doc.end();
        Serial.print("Subscription payload: ");
        Serial.println(payload.c_str());
        sendFrame(payload);
        Serial.println("Sent subscription message");
    }
}

// Generate the GraphQL subscription query with authentication
zap::Str GraphQLSubscriptionClient::getSubscriptionQuery() {
    // The GraphQL query template
    const char* queryTemplate = R"(
    subscription {
      configurationDataChanges(deviceAuth: {
        id: "$serial",
        timestamp: "$timestamp",
        signedIdAndTimestamp: "$signature"
      }) {
        data
        subKey
      }
    }
    )";
    
    // Get current time
    time_t now;
    struct tm timeinfo;
    char timestamp[25];
    
    time(&now);
    gmtime_r(&now, &timeinfo);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S", &timeinfo);
    
    // Get device serial number and signature
    zap::Str serial = crypto_getId();
    zap::Str message = serial + ":" + zap::Str(timestamp);
    zap::Str signature = crypto_create_signature_hex(message.c_str(), PRIVATE_KEY_HEX);
    
    // Replace placeholders in the query
    zap::Str query = zap::Str(queryTemplate);
    query.replace("$serial", serial.c_str());
    query.replace("$timestamp", timestamp);
    query.replace("$signature", signature.c_str());
    
    return query;
}

// Process settings update
void GraphQLSubscriptionClient::handleSettings(JsonParser& configData) {
    Serial.println("Handling settings update");
    // Extract settings data from configData["data"]
    // Update settings in the blackboard
}

// Restart the connection
void GraphQLSubscriptionClient::restart() {
    Serial.println("Restarting WebSocket client");
    stop();
    begin();
}

// Stop the client
void GraphQLSubscriptionClient::stop() {
    if (_isConnected) {
        Serial.println("Closing WebSocket connection");
        
        // Send close frame
        client.write(0x88); // FIN bit set, opcode 0x08 (close)
        client.write(0x02); // Payload length 2
        client.write(0x03); // Status code 1000 (normal closure) high byte
        client.write(0xE8); // Status code 1000 (normal closure) low byte
        
        delay(100); // Give time for the close frame to be sent
    }
    
    _isConnected = false;
    isWebSocketHandshakeDone = false;
    client.stop();
}

