#ifndef DFPLAYER_H
#define DFPLAYER_H

#include <Arduino.h>
#include <SoftwareSerial.h>
#include "config.h"

class DFPlayer {
public:
    DFPlayer(uint8_t rxPin, uint8_t txPin, uint8_t busyPin);
    bool begin(uint8_t volume = 20);

    void playTrack(uint16_t trackNumber);
    void playMp3Folder(uint16_t trackNumber);
    void setVolume(uint8_t volume);
    void stop();
    void pause();
    void resume();

    // Returns true if audio is actively playing
    bool isPlaying();

private:
    uint8_t _busyPin;
    SoftwareSerial _serial;
#if SIMULATION_MODE
    unsigned long _simPlayStart;
    bool _simPlaying;
#endif

    void sendCommand(uint8_t cmd, uint16_t param = 0);
};

#endif // DFPLAYER_H
