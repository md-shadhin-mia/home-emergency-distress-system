#ifndef BUZZER_H
#define BUZZER_H

#include <Arduino.h>

class Buzzer {
public:
    explicit Buzzer(uint8_t pin);
    void begin();
    void update();

    void beep(uint16_t frequency, uint16_t durationMs);
    void playCountdownBeep(uint8_t secondsRemaining);
    void playTriggerChirp();
    void playCancelChirp();
    void playConnectedChirp();
    void playErrorTone();
    void stop();

private:
    uint8_t _pin;
    bool _isPlaying;
    unsigned long _stopTime;
};

#endif // BUZZER_H

