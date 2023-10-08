#pragma once

#include "IScpiDevice.h"
#include <HardwareSerial.h>
#include <string> //std::string
#include <stdint.h> //uint8_t

class ScpiSerialDevice : public IScpiDevice
{
public:
    ScpiSerialDevice(HardwareSerial* serial);

    virtual const char* lastScpiCommand();
    virtual const char* lastScpiCommandReply();
    virtual bool sendScpiCommand(const char* cmdPattern);

protected:
    bool sendScpiCommand(const char* cmdPattern, uint32_t param1);
    bool sendScpiCommand(const char* cmdPattern, uint32_t param1, uint32_t param2);
    bool writeCommandToSerial(char* data, uint8_t len);

protected:
    HardwareSerial* _serial = nullptr;
    String _lastCommand;
    String _lastReply;
};