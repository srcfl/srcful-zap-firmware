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
#include "../zap_log.h" // Added for logging

// Define TAG for logging
static const char* TAG = "config_subscription";

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
    LOG_I(TAG, "GraphQLSubscriptionClient initialized with URL: %s", wsUrl);
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
    
    LOG_D(TAG, "Parsed URL: host=%s, port=%d, path=%s", host.c_str(), port, path.c_str());
}

// Initialize the client
bool GraphQLSubscriptionClient::begin() {
    LOG_I(TAG, "Connecting to WebSocket server: %s:%d%s", host.c_str(), port, path.c_str());

    client.setInsecure(); // Accept all SSL certificates (not recommended for production)
    client.setTimeout(10);
    
    if (client.connect(host.c_str(), port)) {
        LOG_I(TAG, "TCP Connection established");
        _isConnected = true;
        
        // Perform WebSocket handshake
        if (performWebSocketHandshake()) {
            LOG_I(TAG, "WebSocket handshake successful");
            isWebSocketHandshakeDone = true;
            sendConnectionInit();
            return true;
        } 
        else {
            LOG_E(TAG, "WebSocket handshake failed");
            client.stop();
            _isConnected = false;
            return false;
        }
    } 
    else {
        LOG_E(TAG, "TCP Connection failed");
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
            LOG_E(TAG, "Handshake timeout");
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
        LOG_E(TAG, "Handshake failed, server response:");
        LOG_E(TAG, "%s", response.c_str());
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
            LOG_I(TAG, "Attempting to reconnect WebSocket...");
            begin();
        }
        return;
    }
    
    // Check if we need to send a ping
    unsigned long currentMillis = millis();
    if (currentMillis - lastPingTime > PING_INTERVAL) {
        LOG_D(TAG, "Websocket Sending ping");
        sendPing();
        lastPingTime = currentMillis;
        pingPongDiff += 1;
    }
    
    // Check for available data
    if (client.available()) {
        LOG_V(TAG, "Web socket Data available");
        uint8_t buffer[1024]; // Adjust buffer size as needed
        size_t length = client.read(buffer, sizeof(buffer));
        
        
        if (length > 0) {
            processWebSocketData(buffer, length);
        }
    }

    // Check if ping pong has timed out
    if (pingPongDiff > 2) {
        LOG_W(TAG, "Ping pong timeout, two pings sent without response...");
        LOG_W(TAG, "Closing connection...");
        _isConnected = false;
        pingPongDiff = 0;
        isWebSocketHandshakeDone = false;
        client.stop();

        return;
    }
    
    // Check if connection is still alive
    if (!client.connected()) {
        LOG_W(TAG, "Connection lost");
        _isConnected = false;
        isWebSocketHandshakeDone = false;
        client.stop();
    }
}

// Process WebSocket frames
void GraphQLSubscriptionClient::processWebSocketData(uint8_t* buffer, size_t length) {

    // Verbose logging of received hex data
    if (get_log_level() >= ZLOG_LEVEL_VERBOSE) { 
        zap::Str hexData;
        for (size_t i = 0; i < length; i++) {
            char byteHex[4];
            sprintf(byteHex, "%02X ", buffer[i]);
            hexData += byteHex;
        }
        
        LOG_V(TAG, "Received %d bytes:", length);
        LOG_V(TAG, hexData.c_str());
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
                    LOG_I(TAG, "Connection acknowledged, sending subscription");
                    subscribeToSettings();
                } else if (type == "data") {
                    LOG_D(TAG, "Received data: %s", payload);
                    JsonParser configChanges("");
                    if (doc.getObjectByPath("payload.data.configurationDataChanges", configChanges)) {

                        zap::Str subKey;
                        if (configChanges.getString("subKey", subKey)) {
                        
                            if (subKey == SETTINGS_SUBKEY) {
                                LOG_I(TAG, "Handling settings update for subKey: %s", subKey.c_str());
                                handleSettings(configChanges);
                            }
                            else if (subKey == REQUEST_TASK_SUBKEY) {
                                LOG_I(TAG, "Handling request task for subKey: %s", subKey.c_str());
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
            
            LOG_I(TAG, "Received close frame with code: %d", closeCode);
            
            if (payload_length > 2) {
                // Extract close reason if provided
                char* reason = new char[payload_length - 2];
                for (size_t i = 0; i < payload_length - 2; i++) {
                    if (masked) {
                        reason[i] = buffer[header_length + 2 + i] ^ mask[(i + 2) % 4];
                    } else {
                        reason[i] = buffer[header_length + 2 + i];
                    }
                }
                reason[payload_length - 2] = '\0';
                LOG_I(TAG, "Close reason: %s", reason);
                delete[] reason;
            }
            
            stop();
        }
        break;
        case 0x09: // Ping frame, we never seem to get this
        {
            // Respond with pong
            LOG_D(TAG, "Received ping, sending pong");
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
            
            if (pingPongDiff > 0) {
                LOG_D(TAG, "Received valid pong");
                pingPongDiff--;
                lastPongTime = millis();
            } else {
                LOG_W(TAG, "Received unsolicited pong");
            }
        }
        break;
        default:
        {
            LOG_W(TAG, "Received unknown frame type: %d", opcode);
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
            LOG_W(TAG, "Failed to send data byte, retrying...");
            retries++;
        }
    }
    if (retries >= 5) {
        LOG_E(TAG, "Failed to send WebSocket frame after multiple retries.");
    }
}

// Send ping to keep connection alive
void GraphQLSubscriptionClient::sendPing() {
    if (_isConnected && isWebSocketHandshakeDone) {
        // Send WebSocket ping frame
        static uint8_t pingPayload[2] = {0x89, 0x00}; // Empty payload
        client.write(pingPayload, 2); // FIN bit set, opcode 0x09 (ping)
        LOG_D(TAG, "Sent ping");
    }
}

// Send connection initialization message
void GraphQLSubscriptionClient::sendConnectionInit() {
    if (_isConnected && isWebSocketHandshakeDone) {       
        sendFrame("{\"type\":\"connection_init\", \"payload\": {}}");
        LOG_I(TAG, "Sent connection_init message");
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

        JsonBuilder doc;
        doc.beginObject()
            .add("id", "1")
            .add("type", "start")
            .beginObject("payload")
                .add("query", query.c_str())
            .endObject();

        zap::Str payload = doc.end();
        LOG_D(TAG, "Subscription payload: %s", payload.c_str());
        sendFrame(payload);
        LOG_I(TAG, "Sent subscription message");
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
    LOG_I(TAG, "Handling settings update");
    // Extract settings data from configData["data"]
    // Update settings in the blackboard
}

// Restart the connection
void GraphQLSubscriptionClient::restart() {
    LOG_I(TAG, "Restarting WebSocket client");
    stop();
    begin();
}

// Stop the client
void GraphQLSubscriptionClient::stop() {
    if (_isConnected) {
        LOG_I(TAG, "Closing WebSocket connection");
        
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

