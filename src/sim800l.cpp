#include "sim800l.h"

Sim800L::Sim800L(uint8_t resetPin)
    : _resetPin(resetPin)
#if SIMULATION_MODE
    , _simDialTime(0), _simCallActive(false)
#endif
{}

void Sim800L::clearBuffer() {
    while (_serial.available() > 0) {
        _serial.read();
    }
}

void Sim800L::resetHardware() {
#if SIMULATION_MODE
    return;
#else
    pinMode(_resetPin, OUTPUT);
    digitalWrite(_resetPin, LOW);
    delay(150);
    digitalWrite(_resetPin, HIGH);
    pinMode(_resetPin, INPUT_PULLUP); // Allow internal pullup on modem
    delay(3000);                      // Allow modem to complete boot sequence
#endif
}

bool Sim800L::begin(uint32_t baud) {
#if SIMULATION_MODE
    Serial.println(F("[SIMULATION] SIM800L Virtual Cellular Modem initialized."));
    return true;
#else
    _serial.begin(baud);
    delay(500);

    // Test modem responsiveness up to 10 attempts
    bool responding = false;
    for (uint8_t i = 0; i < 10; i++) {
        if (sendCommand("AT", "OK", 1000)) {
            responding = true;
            break;
        }
        delay(500);
    }

    if (!responding) {
        // Try hardware reset if AT communication fails
        resetHardware();
        for (uint8_t i = 0; i < 10; i++) {
            if (sendCommand("AT", "OK", 1000)) {
                responding = true;
                break;
            }
            delay(500);
        }
    }

    if (!responding) return false;

    // Turn off command echo for cleaner response parsing
    sendCommand("ATE0", "OK", 1000);

    // Set SMS mode to text format
    sendCommand("AT+CMGF=1", "OK", 1000);

    // Configure microphone gain channel (Channel 0, Main Audio)
    setMicGain(0, 6);

    return true;
#endif
}

bool Sim800L::isAlive() {
#if SIMULATION_MODE
    return true;
#else
    return sendCommand("AT", "OK", 1000);
#endif
}

bool Sim800L::isSimReady() {
#if SIMULATION_MODE
    return true;
#else
    String resp = sendCommandWithResponse("AT+CPIN?", 2000);
    return resp.indexOf("+CPIN: READY") >= 0;
#endif
}

bool Sim800L::isNetworkRegistered() {
#if SIMULATION_MODE
    return true;
#else
    String resp = sendCommandWithResponse("AT+CREG?", 2000);
    // +CREG: <n>,1 (Registered, home) or +CREG: <n>,5 (Registered, roaming)
    if (resp.indexOf(",1") >= 0 || resp.indexOf(",5") >= 0) {
        return true;
    }
    return false;
#endif
}

int Sim800L::getSignalQuality() {
#if SIMULATION_MODE
    return 26; // Good signal (26/31)
#else
    String resp = sendCommandWithResponse("AT+CSQ", 2000);
    int idx = resp.indexOf("+CSQ: ");
    if (idx >= 0) {
        int commaIdx = resp.indexOf(',', idx);
        if (commaIdx > idx) {
            String valStr = resp.substring(idx + 6, commaIdx);
            valStr.trim();
            return valStr.toInt();
        }
    }
    return -1;
#endif
}

bool Sim800L::setMicGain(uint8_t channel, uint8_t gain) {
#if SIMULATION_MODE
    return true;
#else
    if (gain > 15) gain = 15;
    char cmd[24];
    snprintf(cmd, sizeof(cmd), "AT+CMIC=%u,%u", channel, gain);
    return sendCommand(cmd, "OK", 1500);
#endif
}

bool Sim800L::sendSMS(const char* phoneNumber, const char* message) {
#if SIMULATION_MODE
    Serial.println(F("[SIMULATION] >>> OUTGOING EMERGENCY SMS <<<"));
    Serial.print(F("[SIMULATION] Recipient: "));
    Serial.println(phoneNumber);
    Serial.print(F("[SIMULATION] Body: \""));
    Serial.print(message);
    Serial.println(F("\""));
    delay(400);
    return true;
#else
    clearBuffer();

    char cmd[40];
    snprintf(cmd, sizeof(cmd), "AT+CMGS=\"%s\"", phoneNumber);
    _serial.println(cmd);

    // Wait for the '>' prompt indicating modem is ready for body text
    unsigned long start = millis();
    bool promptReceived = false;
    while (millis() - start < 5000) {
        if (_serial.available() > 0) {
            char c = (char)_serial.read();
            if (c == '>') {
                promptReceived = true;
                break;
            }
        }
    }

    if (!promptReceived) {
        // Cancel SMS attempt
        _serial.write(0x1B); // ESC
        return false;
    }

    // Write SMS body and terminate with Ctrl+Z (ASCII 26)
    _serial.print(message);
    _serial.write(0x1A);

    // Wait up to 15 seconds for +CMGS or OK
    start = millis();
    String resp = "";
    while (millis() - start < 15000) {
        while (_serial.available() > 0) {
            char c = (char)_serial.read();
            resp += c;
        }
        if (resp.indexOf("+CMGS:") >= 0 || resp.indexOf("OK") >= 0) {
            return true;
        }
        if (resp.indexOf("ERROR") >= 0) {
            return false;
        }
    }

    return false;
#endif
}

bool Sim800L::dial(const char* phoneNumber) {
#if SIMULATION_MODE
    Serial.print(F("[SIMULATION] >>> DIALING CALL TO: "));
    Serial.print(phoneNumber);
    Serial.println(F(" <<<"));
    _simDialTime = millis();
    _simCallActive = true;
    return true;
#else
    clearBuffer();
    char cmd[40];
    // Semicolon specifies a voice call (instead of data call)
    snprintf(cmd, sizeof(cmd), "ATD%s;", phoneNumber);
    return sendCommand(cmd, "OK", 3000);
#endif
}

bool Sim800L::hangUp() {
#if SIMULATION_MODE
    Serial.println(F("[SIMULATION] >>> CALL TERMINATED (ATH) <<<"));
    _simCallActive = false;
    return true;
#else
    clearBuffer();
    return sendCommand("ATH", "OK", 2000);
#endif
}

SimCallStatus Sim800L::pollCallStatus() {
#if SIMULATION_MODE
    if (!_simCallActive) {
        return SIM_CALL_IDLE;
    }
    unsigned long elapsed = millis() - _simDialTime;
    if (elapsed < 3000) {
        return SIM_CALL_ALERTING; // Ringing remote dispatcher for 3 seconds
    } else {
        return SIM_CALL_ACTIVE;   // Call answered!
    }
#else
    // Check incoming unsolicited messages first
    while (_serial.available() > 0) {
        String line = readLine(100);
        line.trim();
        if (line.indexOf("BUSY") >= 0) {
            return SIM_CALL_BUSY;
        }
        if (line.indexOf("NO CARRIER") >= 0) {
            return SIM_CALL_NO_CARRIER;
        }
        if (line.indexOf("NO ANSWER") >= 0) {
            return SIM_CALL_NO_ANSWER;
        }
    }

    // Query active calls list via AT+CLCC
    String resp = sendCommandWithResponse("AT+CLCC", 1500);

    if (resp.indexOf("NO CARRIER") >= 0) return SIM_CALL_NO_CARRIER;
    if (resp.indexOf("BUSY") >= 0) return SIM_CALL_BUSY;
    if (resp.indexOf("NO ANSWER") >= 0) return SIM_CALL_NO_ANSWER;

    // +CLCC: <id>,<dir>,<stat>,<mode>,<mpty>...
    int idx = resp.indexOf("+CLCC: ");
    if (idx >= 0) {
        int firstComma = resp.indexOf(',', idx);
        int secondComma = resp.indexOf(',', firstComma + 1);
        int thirdComma = resp.indexOf(',', secondComma + 1);

        if (secondComma > 0 && thirdComma > secondComma) {
            String statStr = resp.substring(secondComma + 1, thirdComma);
            statStr.trim();
            int stat = statStr.toInt();

            switch (stat) {
                case 0: return SIM_CALL_ACTIVE;   // Connected / Answered
                case 2: return SIM_CALL_DIALING;  // Dialing
                case 3: return SIM_CALL_ALERTING; // Ringing
                case 6: return SIM_CALL_NO_CARRIER;
                default: return SIM_CALL_ALERTING;
            }
        }
    }

    // If AT+CLCC returns OK with no active calls listed, the call has ended
    if (resp.indexOf("OK") >= 0 && idx < 0) {
        return SIM_CALL_IDLE;
    }

    return SIM_CALL_IDLE;
#endif
}

bool Sim800L::sendCommand(const char* cmd, const char* expectedReply, uint32_t timeoutMs) {
    clearBuffer();
    _serial.println(cmd);

    unsigned long start = millis();
    String resp = "";

    while (millis() - start < timeoutMs) {
        while (_serial.available() > 0) {
            char c = (char)_serial.read();
            resp += c;
        }
        if (resp.indexOf(expectedReply) >= 0) {
            return true;
        }
        if (resp.indexOf("ERROR") >= 0) {
            return false;
        }
    }

    return false;
}

String Sim800L::sendCommandWithResponse(const char* cmd, uint32_t timeoutMs) {
    clearBuffer();
    _serial.println(cmd);

    unsigned long start = millis();
    String resp = "";

    while (millis() - start < timeoutMs) {
        while (_serial.available() > 0) {
            char c = (char)_serial.read();
            resp += c;
        }
        if (resp.indexOf("OK") >= 0 || resp.indexOf("ERROR") >= 0) {
            break;
        }
    }

    return resp;
}

String Sim800L::readLine(uint32_t timeoutMs) {
    String line = "";
    unsigned long start = millis();
    while (millis() - start < timeoutMs) {
        if (_serial.available() > 0) {
            char c = (char)_serial.read();
            if (c == '\n') break;
            if (c != '\r') line += c;
        }
    }
    return line;
}
