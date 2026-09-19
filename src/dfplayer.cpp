#include "dfplayer.h"

// Command definitions
#define CMD_PLAY_TRACK      0x03
#define CMD_SET_VOLUME      0x06
#define CMD_RESET           0x0C
#define CMD_PLAY            0x0D
#define CMD_PAUSE           0x0E
#define CMD_STOP            0x16
#define CMD_PLAY_MP3_FOLDER 0x12

DFPlayer::DFPlayer(uint8_t rxPin, uint8_t txPin, uint8_t busyPin)
    : _busyPin(busyPin), _serial(rxPin, txPin)
#if SIMULATION_MODE
    , _simPlayStart(0), _simPlaying(false)
#endif
{}

bool DFPlayer::begin(uint8_t volume) {
    pinMode(_busyPin, INPUT_PULLUP);
#if SIMULATION_MODE
    Serial.println(F("[SIMULATION] DFPlayer Mini Virtual Audio Decoder initialized."));
    _simPlaying = false;
    return true;
#else
    _serial.begin(9600);
    delay(200);

    // Set initial volume (clamped between 0 and 30)
    setVolume(volume);
    delay(100);

    return true;
#endif
}

void DFPlayer::sendCommand(uint8_t cmd, uint16_t param) {
    uint8_t paramHigh = (uint8_t)(param >> 8);
    uint8_t paramLow  = (uint8_t)(param & 0xFF);

    // Calculate checksum: -(Version + Length + Command + Feedback + ParamHigh + ParamLow)
    uint16_t checksum = -(0xFF + 0x06 + cmd + 0x00 + paramHigh + paramLow);
    uint8_t checkHigh = (uint8_t)(checksum >> 8);
    uint8_t checkLow  = (uint8_t)(checksum & 0xFF);

    uint8_t buffer[10] = {
        0x7E,       // Start byte
        0xFF,       // Version
        0x06,       // Length
        cmd,        // Command
        0x00,       // Feedback (0x00 = off)
        paramHigh,  // Parameter High byte
        paramLow,   // Parameter Low byte
        checkHigh,  // Checksum High byte
        checkLow,   // Checksum Low byte
        0xEF        // End byte
    };

    _serial.write(buffer, 10);
    _serial.flush();
}

void DFPlayer::setVolume(uint8_t volume) {
    if (volume > 30) volume = 30;
#if !SIMULATION_MODE
    sendCommand(CMD_SET_VOLUME, volume);
#endif
}

void DFPlayer::playTrack(uint16_t trackNumber) {
#if SIMULATION_MODE
    Serial.print(F("[SIMULATION] >>> Playing Audio Track #"));
    Serial.print(trackNumber);
    Serial.println(F(" into call line <<<"));
    _simPlayStart = millis();
    _simPlaying = true;
#else
    sendCommand(CMD_PLAY_TRACK, trackNumber);
#endif
}

void DFPlayer::playMp3Folder(uint16_t trackNumber) {
#if SIMULATION_MODE
    Serial.print(F("[SIMULATION] >>> Playing Audio /mp3/000"));
    Serial.print(trackNumber);
    Serial.println(F(".mp3 into call line <<<"));
    _simPlayStart = millis();
    _simPlaying = true;
#else
    sendCommand(CMD_PLAY_MP3_FOLDER, trackNumber);
#endif
}

void DFPlayer::stop() {
#if SIMULATION_MODE
    _simPlaying = false;
#else
    sendCommand(CMD_STOP);
#endif
}

void DFPlayer::pause() {
#if SIMULATION_MODE
    _simPlaying = false;
#else
    sendCommand(CMD_PAUSE);
#endif
}

void DFPlayer::resume() {
#if SIMULATION_MODE
    _simPlaying = true;
#else
    sendCommand(CMD_PLAY);
#endif
}

bool DFPlayer::isPlaying() {
#if SIMULATION_MODE
    if (_simPlaying) {
        // Simulate a 4-second distress message playback
        if (millis() - _simPlayStart < 4000) {
            return true;
        } else {
            _simPlaying = false;
            return false;
        }
    }
    return false;
#else
    // Hardware BUSY pin: LOW indicates audio playback in progress, HIGH indicates idle
    return (digitalRead(_busyPin) == LOW);
#endif
}
