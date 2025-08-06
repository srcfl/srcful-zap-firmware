# MQTT v5 Client Integration

This firmware now includes a simple MQTT v5 client that integrates with the existing WiFi and task management system.

## Features

- **MQTT v5 Protocol Support**: Uses the arduino-mqtt library for modern MQTT features
- **Task-based Architecture**: Runs in its own FreeRTOS task, non-blocking
- **Automatic Connection Management**: Connects when WiFi is available, reconnects on failures
- **SSL/TLS Support**: Configurable SSL encryption for secure connections
- **Flexible Authentication**: Support for username/password authentication
- **Built-in Message Handling**: Simple command processing with extensible callback system
- **System Integration**: Publishes device status, heartbeats, and sensor data
- **Robust Error Handling**: Comprehensive logging and connection state management

## Configuration

Edit the MQTT settings in `src/config.cpp`:

```cpp
// MQTT Configuration - Configure these according to your MQTT broker
const char* MQTT_SERVER = "";                    // MQTT broker hostname/IP (empty = disabled)
const uint16_t MQTT_PORT = 1883;                 // MQTT broker port (1883 for non-SSL, 8883 for SSL)
const bool MQTT_USE_SSL = false;                 // Use SSL/TLS connection
const char* MQTT_USERNAME = "";                  // MQTT username (empty if no auth required)
const char* MQTT_PASSWORD = "";                  // MQTT password (empty if no auth required) 
const char* MQTT_CLIENT_ID = "zap_device";       // MQTT client ID
const char* MQTT_SUBSCRIBE_TOPIC = "zap/commands"; // Topic to subscribe to for incoming commands
```

## Example Configurations

### 1. Local Mosquitto Broker (No Authentication)
```cpp
const char* MQTT_SERVER = "192.168.1.100";
const uint16_t MQTT_PORT = 1883;
const bool MQTT_USE_SSL = false;
const char* MQTT_USERNAME = "";
const char* MQTT_PASSWORD = "";
const char* MQTT_CLIENT_ID = "zap_device";
const char* MQTT_SUBSCRIBE_TOPIC = "home/zap/commands";
```

### 2. HiveMQ Cloud (Free Tier with Authentication)
```cpp
const char* MQTT_SERVER = "your-cluster.s1.eu.hivemq.cloud";
const uint16_t MQTT_PORT = 8883;
const bool MQTT_USE_SSL = true;
const char* MQTT_USERNAME = "your_username";
const char* MQTT_PASSWORD = "your_password";
const char* MQTT_CLIENT_ID = "zap_device_001";
const char* MQTT_SUBSCRIBE_TOPIC = "devices/zap/commands";
```

### 3. AWS IoT Core
```cpp
const char* MQTT_SERVER = "your-endpoint.iot.region.amazonaws.com";
const uint16_t MQTT_PORT = 8883;
const bool MQTT_USE_SSL = true;
const char* MQTT_USERNAME = "";  // AWS IoT uses certificate authentication
const char* MQTT_PASSWORD = "";
const char* MQTT_CLIENT_ID = "zap_device_001";
const char* MQTT_SUBSCRIBE_TOPIC = "devices/zap/commands";
```

## Message Topics

The MQTT client automatically publishes to several topics:

### Outgoing Topics (Published by Device)
- `zap/{client_id}/status` - Device status messages
- `zap/{client_id}/heartbeat` - Periodic heartbeat with system info
- `zap/{client_id}/sensor/{sensor_type}` - Sensor data in JSON format

### Incoming Topics (Subscribed by Device)
- Configured via `MQTT_SUBSCRIBE_TOPIC` (default: `zap/commands`)

## Built-in Commands

Send these simple text commands to the subscribed topic:

- `status` - Device responds with current status
- `heartbeat` - Forces immediate heartbeat message
- `reboot` - Logs reboot command (extend to actually reboot)

You can also send JSON commands for more complex operations.

## Example Usage

### Publishing Custom Data
```cpp
// From anywhere in your code
extern ZapMQTTClient mqttClient;

// Simple status
mqttClient.publishStatus("sensor_reading_complete");

// Sensor data (auto-formatted as JSON)
mqttClient.publishSensorData("temperature", "23.5");

// Raw topic/payload
mqttClient.publish("custom/topic", "custom message", false);
```

### Testing with Mosquitto Tools

1. **Subscribe to all device messages:**
   ```bash
   mosquitto_sub -h your-broker -t "zap/+/+"
   ```

2. **Send a status command:**
   ```bash
   mosquitto_pub -h your-broker -t "zap/commands" -m "status"
   ```

3. **Send a heartbeat request:**
   ```bash
   mosquitto_pub -h your-broker -t "zap/commands" -m "heartbeat"
   ```

## Integration Details

The MQTT client is integrated into the existing firmware architecture:

1. **Initialization**: Configured and started in `setup()` if `MQTT_SERVER` is not empty
2. **Task Management**: Runs in its own FreeRTOS task with automatic lifecycle management
3. **WiFi Integration**: Only operates when WiFi is connected, handles disconnections gracefully
4. **Logging**: Uses the existing `zap_log` system for consistent debug output
5. **Non-blocking**: Doesn't interfere with existing BLE, HTTP, or data collection tasks

## Monitoring and Debugging

Monitor MQTT operations via serial console:

```
[mqtt_client] MQTT server set to broker.example.com:1883 (SSL: no)
[mqtt_client] MQTT task created successfully
[mqtt_client] MQTT client task started
[mqtt_client] Attempting to connect to MQTT broker: broker.example.com:1883
[mqtt_client] Connected to MQTT broker
[mqtt_client] Subscribed to topic: zap/commands
[mqtt_client] Published to zap/zap_device/heartbeat: {"uptime":45230,"heap":234567}
[mqtt_client] Received message on topic 'zap/commands': status
```

## Customization

### Adding Custom Commands
Modify `mqttMessageReceived()` in `src/mqtt/mqtt_client.cpp`:

```cpp
void ZapMQTTClient::mqttMessageReceived(String &topic, String &payload) {
    // ... existing code ...
    
    // Add your custom commands
    if (payload == "get_config") {
        // Publish current configuration
        instance->publish("zap/config", getCurrentConfig().c_str(), false);
    } else if (payload.startsWith("set_")) {
        // Handle configuration changes
        handleConfigCommand(payload);
    }
}
```

### Publishing Additional Sensor Data
Add to the main loop in `main.cpp`:

```cpp
// In the WiFi connected section
if (mqttClient.isConnected() && (currentTime - lastMqttPublish > 60000)) {
    // ... existing code ...
    
    // Add your sensor readings
    mqttClient.publishSensorData("wifi_rssi", String(WiFi.RSSI()).c_str());
    mqttClient.publishSensorData("uptime", String(millis()).c_str());
    
    // Read from P1 meter data if available
    // mqttClient.publishSensorData("power", currentPowerReading.c_str());
}
```

## Security Notes

- For production use, always enable SSL (`MQTT_USE_SSL = true`)
- Use strong, unique passwords for MQTT authentication
- Consider certificate-based authentication for enhanced security
- The current SSL implementation uses `setInsecure()` - for production, implement proper certificate validation

## Troubleshooting

### Common Issues

1. **MQTT not connecting**: Check server/port/credentials in config
2. **SSL connection fails**: Verify SSL port (usually 8883) and broker SSL support
3. **Messages not received**: Confirm topic subscription and broker permissions
4. **High memory usage**: Adjust buffer size in constructor or reduce publish frequency

### Debug Steps

1. Enable verbose logging by modifying log levels in `zap_log.h`
2. Use MQTT client tools to test broker connectivity independently
3. Monitor heap usage with built-in heartbeat messages
4. Check WiFi connection stability if MQTT keeps disconnecting

## Library Dependencies

The MQTT functionality adds one dependency to `platformio.ini`:

```ini
lib_deps =
    h2zero/NimBLE-Arduino @ ^1.4.1
    emelianov/modbus-esp8266 @ ^4.1.0
    256dpi/arduino-mqtt @ ^2.5.2  # <-- MQTT library
```

This library provides modern MQTT v5 support with a simple API and good ESP32 compatibility.
