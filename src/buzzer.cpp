#include "buzzer.h"

Buzzer::Buzzer(uint8_t pin)
    : _pin(pin), _isPlaying(false), _stopTime(0) {}

void Buzzer::begin() {
    pinMode(_pin, OUTPUT);
    noTone(_pin);
}

void Buzzer::update() {
    if (_isPlaying && millis() >= _stopTime) {
        noTone(_pin);
        _isPlaying = false;
    }
}

void Buzzer::beep(uint16_t frequency, uint16_t durationMs) {
    tone(_pin, frequency);
    _isPlaying = true;
    _stopTime = millis() + durationMs;
}

void Buzzer::playCountdownBeep(uint8_t secondsRemaining) {
    // Higher pitch as seconds run out
    uint16_t freq = 2000 + (5 - secondsRemaining) * 200;
    beep(freq, 120);
}

void Buzzer::playTriggerChirp() {
    tone(_pin, 2400);
    delay(80);
    tone(_pin, 3200);
    delay(100);
    noTone(_pin);
    _isPlaying = false;
}

void Buzzer::playCancelChirp() {
    tone(_pin, 2800);
    delay(100);
    tone(_pin, 1400);
    delay(150);
    noTone(_pin);
    _isPlaying = false;
}

void Buzzer::playConnectedChirp() {
    tone(_pin, 1800);
    delay(80);
    tone(_pin, 2400);
    delay(80);
    tone(_pin, 3000);
    delay(120);
    noTone(_pin);
    _isPlaying = false;
}

void Buzzer::playErrorTone() {
    tone(_pin, 600);
    delay(300);
    noTone(_pin);
    _isPlaying = false;
}

void Buzzer::stop() {
    noTone(_pin);
    _isPlaying = false;
}

