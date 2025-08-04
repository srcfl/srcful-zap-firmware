#pragma once

#include "endpoint_types.h"

class ModbusTcpHandler : public EndpointFunction {
public:
    ModbusTcpHandler() {}
    EndpointResponse handle(const zap::Str& contents) override;

private:
    struct ModbusRequest {
        zap::Str ip;
        uint16_t port;
        uint8_t slaveId;
        uint16_t startRegister;
        uint16_t numRegisters;
        uint8_t functionCode;
        
        bool isValid() const {
            return ip.length() > 0 && 
                   port > 0 && port <= 65535 &&
                   slaveId <= 247 &&
                   numRegisters > 0 && numRegisters <= 125 &&
                   (functionCode == 3 || functionCode == 4 || functionCode == 16); // Read Holding/Input Registers, Write Multiple Registers
        }
    };
};
