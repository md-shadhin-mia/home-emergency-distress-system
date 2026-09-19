#ifndef SIM800L_H
#define SIM800L_H

#include <Arduino.h>
#include <AltSoftSerial.h>
#include "config.h"

enum SimCallStatus {
    SIM_CALL_IDLE,
    SIM_CALL_DIALING,
    SIM_CALL_ALERTING,      // Ringing on remote end
    SIM_CALL_ACTIVE,        // Call answered and connected
    SIM_CALL_BUSY,
    SIM_CALL_NO_ANSWER,
    SIM_CALL_NO_CARRIER,    // Call disconnected or rejected
    SIM_CALL_ERROR
};

class Sim800L {
public:
    explicit Sim800L(uint8_t resetPin);
    bool begin(uint32_t baud = 9600);
    void resetHardware();

    // Health & Network
    bool isAlive();
    bool isSimReady();
    bool isNetworkRegistered();
    int getSignalQuality(); // CSQ: 0-31 (99 = unknown)

    // Audio & Gain Configuration
    bool setMicGain(uint8_t channel, uint8_t gain);

    // SMS Operations
    bool sendSMS(const char* phoneNumber, const char* message);

    // Call Operations
    bool dial(const char* phoneNumber);
    bool hangUp();
    SimCallStatus pollCallStatus();

    // Low-level command execution
    bool sendCommand(const char* cmd, const char* expectedReply, uint32_t timeoutMs = 2000);
    String sendCommandWithResponse(const char* cmd, uint32_t timeoutMs = 2000);

private:
    uint8_t _resetPin;
    AltSoftSerial _serial;
#if SIMULATION_MODE
    unsigned long _simDialTime;
    bool _simCallActive;
#endif

    void clearBuffer();
    String readLine(uint32_t timeoutMs);
};

#endif // SIM800L_H

