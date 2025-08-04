#include "modbus_endpoint_handlers.h"
#include "json_light/json_light.h"
#include "zap_log.h"
#include <ModbusIP_ESP8266.h>
#include <WiFi.h>

static constexpr LogTag TAG_MODBUS = LogTag("modbus_endpoint", ZLOG_LEVEL_DEBUG);

EndpointResponse ModbusTcpHandler::handle(const zap::Str& contents) {
    EndpointResponse response;
    response.contentType = "application/json";
    
    // Parse JSON body parameters
    JsonParser parser(contents.c_str());
    ModbusRequest modbusReq;
    
    char ipBuffer[16];
    if (!parser.getString("ip", ipBuffer, sizeof(ipBuffer))) {
        response.statusCode = 400;
        JsonBuilder json;
        json.beginObject()
            .add("status", "error")
            .add("message", "Missing 'ip' parameter");
        response.data = json.end();
        return response;
    }
    modbusReq.ip = zap::Str(ipBuffer);
    
    int port;
    if (!parser.getInt("port", port)) {
        port = 502; // Default Modbus port
    }
    modbusReq.port = (uint16_t)port;
    
    int slave;
    if (!parser.getInt("slave", slave)) {
        response.statusCode = 400;
        JsonBuilder json;
        json.beginObject()
            .add("status", "error")
            .add("message", "Missing 'slave' parameter");
        response.data = json.end();
        return response;
    }
    modbusReq.slaveId = (uint8_t)slave;
    
    int start;
    if (!parser.getInt("start", start)) {
        response.statusCode = 400;
        JsonBuilder json;
        json.beginObject()
            .add("status", "error")
            .add("message", "Missing 'start' parameter");
        response.data = json.end();
        return response;
    }
    modbusReq.startRegister = (uint16_t)start;
    
    int func;
    if (!parser.getInt("func", func)) {
        response.statusCode = 400;
        JsonBuilder json;
        json.beginObject()
            .add("status", "error")
            .add("message", "Missing 'func' parameter");
        response.data = json.end();
        return response;
    }
    modbusReq.functionCode = (uint8_t)func;
    
    int num;
    if (!parser.getInt("num", num)) {
        // For write operations, we'll determine num from the values array length
        // For read operations, num is required
        if (func != 16) {
            response.statusCode = 400;
            JsonBuilder json;
            json.beginObject()
                .add("status", "error")
                .add("message", "Missing 'num' parameter for read operation");
            response.data = json.end();
            return response;
        }
        num = 0; // Will be set from values array length for write operations
    }
    modbusReq.numRegisters = (uint16_t)num;
    
    // Check if this is a write operation (function code 16)
    bool isWrite = (modbusReq.functionCode == 16);
    uint16_t* writeValues = nullptr;
    uint16_t writeCount = 0;
    
    if (isWrite) {
        // For write operations, parse the values array
        // First, we need to manually parse the values array since JsonParser doesn't have array support
        const char* content = contents.c_str();
        const char* valuesStart = strstr(content, "\"values\"");
        if (!valuesStart) {
            response.statusCode = 400;
            JsonBuilder json;
            json.beginObject()
                .add("status", "error")
                .add("message", "Missing 'values' array for write operation");
            response.data = json.end();
            return response;
        }
        
        // Find the array start
        const char* arrayStart = strchr(valuesStart, '[');
        if (!arrayStart) {
            response.statusCode = 400;
            JsonBuilder json;
            json.beginObject()
                .add("status", "error")
                .add("message", "Invalid 'values' array format");
            response.data = json.end();
            return response;
        }
        
        // Parse values from the array
        uint16_t maxValues = 125; // Maximum Modbus registers in one transaction
        writeValues = new uint16_t[maxValues];
        const char* ptr = arrayStart + 1;
        writeCount = 0;
        
        while (*ptr && *ptr != ']' && writeCount < maxValues) {
            // Skip whitespace and commas
            while (*ptr && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == ',')) ptr++;
            if (*ptr == ']') break;
            
            // Parse number
            int value = atoi(ptr);
            writeValues[writeCount++] = (uint16_t)value;
            
            // Skip to next comma or end
            while (*ptr && *ptr != ',' && *ptr != ']') ptr++;
        }
        
        // Set numRegisters from the actual values array length
        modbusReq.numRegisters = writeCount;
        
        if (writeCount == 0) {
            delete[] writeValues;
            response.statusCode = 400;
            JsonBuilder json;
            json.beginObject()
                .add("status", "error")
                .add("message", "Empty values array for write operation");
            response.data = json.end();
            return response;
        }
    }
    
    if (!modbusReq.isValid()) {
        if (writeValues) delete[] writeValues;
        response.statusCode = 400;
        JsonBuilder json;
        json.beginObject()
            .add("status", "error")
            .add("message", "Invalid parameter values");
        response.data = json.end();
        return response;
    }
    
    LOG_TI(TAG_MODBUS, "Modbus %s request: %s:%d, slave=%d, start=%d, num=%d, func=%d", 
           isWrite ? "write" : "read", modbusReq.ip.c_str(), modbusReq.port, modbusReq.slaveId, 
           modbusReq.startRegister, modbusReq.numRegisters, modbusReq.functionCode);
    
    // Create Modbus client
    ModbusIP mb;
    
    // Parse IP address
    IPAddress serverIP;
    if (!serverIP.fromString(modbusReq.ip.c_str())) {
        if (writeValues) delete[] writeValues;
        response.statusCode = 400;
        JsonBuilder json;
        json.beginObject()
            .add("status", "error")
            .add("message", "Invalid IP address format");
        response.data = json.end();
        return response;
    }
    
    // Connect to Modbus server
    if (!mb.connect(serverIP, modbusReq.port)) {
        if (writeValues) delete[] writeValues;
        LOG_TE(TAG_MODBUS, "Failed to connect to Modbus TCP server");
        response.statusCode = 500;
        JsonBuilder json;
        json.beginObject()
            .add("status", "error")
            .add("message", "Failed to connect to Modbus TCP server");
        response.data = json.end();
        return response;
    }
    
    // Prepare response buffer for read operations
    uint16_t* readValues = nullptr;
    bool success = false;
    
    // Send Modbus request based on function code
    if (modbusReq.functionCode == 3) {
        // Read Holding Registers
        readValues = new uint16_t[modbusReq.numRegisters];
        success = mb.readHreg(serverIP, modbusReq.startRegister, readValues, modbusReq.numRegisters, 
                             nullptr, modbusReq.slaveId);
    } else if (modbusReq.functionCode == 4) {
        // Read Input Registers
        readValues = new uint16_t[modbusReq.numRegisters];
        success = mb.readIreg(serverIP, modbusReq.startRegister, readValues, modbusReq.numRegisters, 
                             nullptr, modbusReq.slaveId);
    } else if (modbusReq.functionCode == 16) {
        // Write Multiple Registers
        success = mb.writeHreg(serverIP, modbusReq.startRegister, writeValues, modbusReq.numRegisters, 
                              nullptr, modbusReq.slaveId);
    } else {
        if (writeValues) delete[] writeValues;
        if (readValues) delete[] readValues;
        response.statusCode = 400;
        JsonBuilder json;
        json.beginObject()
            .add("status", "error")
            .add("message", "Unsupported function code. Supported: 3 (Read Holding), 4 (Read Input), 16 (Write Multiple)");
        response.data = json.end();
        return response;
    }
    
    if (!success) {
        if (writeValues) delete[] writeValues;
        if (readValues) delete[] readValues;
        LOG_TE(TAG_MODBUS, "Modbus request failed");
        response.statusCode = 500;
        JsonBuilder json;
        json.beginObject()
            .add("status", "error")
            .add("message", "Modbus request failed");
        response.data = json.end();
        return response;
    }
    
    // Build successful response
    response.statusCode = 200;
    
    JsonBuilder json;
    json.beginObject()
        .add("status", "success")
        .add("ip", modbusReq.ip.c_str())
        .add("port", modbusReq.port)
        .add("slave", modbusReq.slaveId)
        .add("start", modbusReq.startRegister)
        .add("count", modbusReq.numRegisters)
        .add("function", modbusReq.functionCode);
    
    if (isWrite) {
        // For write operations, include the written values
        zap::Str valuesStr("[");
        for (uint16_t i = 0; i < modbusReq.numRegisters; i++) {
            if (i > 0) valuesStr += ",";
            valuesStr += zap::Str(writeValues[i]);
        }
        valuesStr += "]";
        
        // Add values array manually by building the JSON string
        zap::Str jsonStr = json.end();
        // Remove the closing brace and add values manually
        jsonStr = jsonStr.substring(0, jsonStr.length() - 1); // Remove '}'
        jsonStr += ",\"written_values\":";
        jsonStr += valuesStr;
        jsonStr += "}";
        
        response.data = jsonStr;
        LOG_TI(TAG_MODBUS, "Successfully wrote %d registers", modbusReq.numRegisters);
    } else {
        // For read operations, include the read values
        zap::Str valuesStr("[");
        for (uint16_t i = 0; i < modbusReq.numRegisters; i++) {
            if (i > 0) valuesStr += ",";
            valuesStr += zap::Str(readValues[i]);
        }
        valuesStr += "]";
        
        // Add values array manually by building the JSON string
        zap::Str jsonStr = json.end();
        // Remove the closing brace and add values manually
        jsonStr = jsonStr.substring(0, jsonStr.length() - 1); // Remove '}'
        jsonStr += ",\"values\":";
        jsonStr += valuesStr;
        jsonStr += "}";
        
        response.data = jsonStr;
        LOG_TI(TAG_MODBUS, "Successfully read %d registers", modbusReq.numRegisters);
    }
    
    // Clean up allocated memory
    if (writeValues) delete[] writeValues;
    if (readValues) delete[] readValues;
    
    return response;
}
