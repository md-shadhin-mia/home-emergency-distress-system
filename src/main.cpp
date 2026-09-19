#include <Arduino.h>
#include "config.h"
#include "buzzer.h"
#include "dfplayer.h"
#include "sim800l.h"

// ==============================================================================
// GLOBAL OBJECTS & ENUMS
// ==============================================================================
Buzzer buzzer(PIN_BUZZER);
DFPlayer dfPlayer(PIN_DF_RX, PIN_DF_TX, PIN_DF_BUSY);
Sim800L sim800l(PIN_SIM_RST);

enum SystemState {
    STATE_INIT,
    STATE_IDLE,
    STATE_COUNTDOWN,
    STATE_SEND_SMS,
    STATE_DIAL_CALL,
    STATE_CALL_WAIT_ANSWER,
    STATE_PLAY_AUDIO,
    STATE_CALL_RETRY,
    STATE_FAULT
};

enum EmergencyType {
    EMERGENCY_NONE,
    EMERGENCY_POLICE,
    EMERGENCY_FIRE,
    EMERGENCY_MEDICAL
};

SystemState currentState = STATE_INIT;
EmergencyType activeEmergency = EMERGENCY_NONE;

// Timing and counters
unsigned long stateTimer = 0;
unsigned long lastStatusCheck = 0;
unsigned long lastBlinkTime = 0;
unsigned long lastBeepTime = 0;
uint8_t dialRetryCount = 0;
uint8_t audioRepeatCount = 0;
bool isAudioCurrentlyPlaying = false;
bool greenLedState = false;

// Button debounce tracking
struct Button {
    uint8_t pin;
    bool lastState;
    bool currentState;
    unsigned long lastDebounceTime;
};

Button btnPolice  = { PIN_BTN_POLICE,  HIGH, HIGH, 0 };
Button btnFire    = { PIN_BTN_FIRE,    HIGH, HIGH, 0 };
Button btnMedical = { PIN_BTN_MEDICAL, HIGH, HIGH, 0 };
Button btnCancel  = { PIN_BTN_CANCEL,  HIGH, HIGH, 0 };

// ==============================================================================
// FUNCTION DECLARATIONS
// ==============================================================================
bool checkButtonPressed(Button& btn);
void updateLeds();
void handleIdle();
void handleCountdown();
void handleSendSms();
void handleDialCall();
void handleWaitAnswer();
void handlePlayAudio();
void handleCallRetry();
const char* getEmergencyName(EmergencyType type);
const char* getEmergencyNumber(EmergencyType type);
uint16_t getEmergencyTrack(EmergencyType type);

// ==============================================================================
// SETUP
// ==============================================================================
void setup() {
    Serial.begin(SERIAL_DEBUG_BAUD);
    while (!Serial && millis() < 2000); // Short wait for USB Serial

    Serial.println(F("\n=============================================="));
    Serial.println(F(" Standalone Home Emergency Dispatch System    "));
    Serial.println(F("=============================================="));

    // Configure LEDs and Buzzer
    pinMode(PIN_LED_GREEN, OUTPUT);
    pinMode(PIN_LED_RED, OUTPUT);
    digitalWrite(PIN_LED_GREEN, LOW);
    digitalWrite(PIN_LED_RED, HIGH); // Red ON during startup
    buzzer.begin();

    // Configure Button Inputs with internal pull-ups (Active LOW)
    pinMode(PIN_BTN_POLICE,  INPUT_PULLUP);
    pinMode(PIN_BTN_FIRE,    INPUT_PULLUP);
    pinMode(PIN_BTN_MEDICAL, INPUT_PULLUP);
    pinMode(PIN_BTN_CANCEL,  INPUT_PULLUP);

    // Initialize Audio Subsystem
    Serial.print(F("[AUDIO] Initializing DFPlayer Mini... "));
    if (dfPlayer.begin(DFPLAYER_VOLUME)) {
        Serial.println(F("OK"));
    } else {
        Serial.println(F("FAILED (check wiring/SD card)"));
    }

    // Initialize Cellular Subsystem
    Serial.print(F("[GSM] Initializing SIM800L module... "));
    if (sim800l.begin(SIM800L_BAUD)) {
        Serial.println(F("OK"));
    } else {
        Serial.println(F("FAILED (Modem not responding)"));
        currentState = STATE_FAULT;
        return;
    }

    // Check SIM Card
    Serial.print(F("[GSM] Checking SIM card status... "));
    if (sim800l.isSimReady()) {
        Serial.println(F("READY"));
    } else {
        Serial.println(F("NO SIM / PIN LOCKED"));
        currentState = STATE_FAULT;
        return;
    }

    // Wait for Network Registration (up to 30 seconds)
    Serial.print(F("[GSM] Waiting for cellular network registration... "));
    unsigned long regStart = millis();
    bool registered = false;
    while (millis() - regStart < 30000) {
        if (sim800l.isNetworkRegistered()) {
            registered = true;
            break;
        }
        delay(1000);
        Serial.print(F("."));
    }

    if (registered) {
        int csq = sim800l.getSignalQuality();
        Serial.print(F(" REGISTERED (CSQ: "));
        Serial.print(csq);
        Serial.println(F(" / 31)"));

        digitalWrite(PIN_LED_RED, LOW);
        digitalWrite(PIN_LED_GREEN, HIGH);
        buzzer.playTriggerChirp();
        currentState = STATE_IDLE;
        Serial.println(F("[SYSTEM] Armed and ready for emergency distress input."));
    } else {
        Serial.println(F(" TIMEOUT (Network registration failed)"));
        currentState = STATE_FAULT;
    }
}

// ==============================================================================
// MAIN LOOP
// ==============================================================================
void loop() {
    buzzer.update();
    updateLeds();

    switch (currentState) {
        case STATE_IDLE:
            handleIdle();
            break;

        case STATE_COUNTDOWN:
            handleCountdown();
            break;

        case STATE_SEND_SMS:
            handleSendSms();
            break;

        case STATE_DIAL_CALL:
            handleDialCall();
            break;

        case STATE_CALL_WAIT_ANSWER:
            handleWaitAnswer();
            break;

        case STATE_PLAY_AUDIO:
            handlePlayAudio();
            break;

        case STATE_CALL_RETRY:
            handleCallRetry();
            break;

        case STATE_FAULT:
            // Retry network check every 10 seconds
            if (millis() - lastStatusCheck >= 10000) {
                lastStatusCheck = millis();
                if (sim800l.isAlive() && sim800l.isNetworkRegistered()) {
                    Serial.println(F("[SYSTEM] Network re-established. Arming system."));
                    currentState = STATE_IDLE;
                }
            }
            break;

        default:
            currentState = STATE_IDLE;
            break;
    }
}

// ==============================================================================
// STATE HANDLERS
// ==============================================================================

void handleIdle() {
    // Check Emergency Buttons
    if (checkButtonPressed(btnPolice)) {
        activeEmergency = EMERGENCY_POLICE;
        Serial.println(F("\n>>> POLICE BUTTON TRIGGERED <<<"));
        stateTimer = millis();
        lastBeepTime = 0;
        currentState = STATE_COUNTDOWN;
        return;
    }

    if (checkButtonPressed(btnFire)) {
        activeEmergency = EMERGENCY_FIRE;
        Serial.println(F("\n>>> FIRE BUTTON TRIGGERED <<<"));
        stateTimer = millis();
        lastBeepTime = 0;
        currentState = STATE_COUNTDOWN;
        return;
    }

    if (checkButtonPressed(btnMedical)) {
        activeEmergency = EMERGENCY_MEDICAL;
        Serial.println(F("\n>>> MEDICAL BUTTON TRIGGERED <<<"));
        stateTimer = millis();
        lastBeepTime = 0;
        currentState = STATE_COUNTDOWN;
        return;
    }

    // Periodic network registration health check every 30 seconds
    if (millis() - lastStatusCheck >= 30000) {
        lastStatusCheck = millis();
        if (!sim800l.isNetworkRegistered()) {
            Serial.println(F("[WARNING] Cellular network registration lost!"));
            currentState = STATE_FAULT;
        }
    }
}

void handleCountdown() {
    unsigned long elapsed = millis() - stateTimer;

    // Check for Cancel Button press
    if (checkButtonPressed(btnCancel)) {
        Serial.println(F("[ABORT] Cancel button pressed! Emergency aborted."));
        buzzer.playCancelChirp();
        activeEmergency = EMERGENCY_NONE;
        digitalWrite(PIN_LED_RED, LOW);
        currentState = STATE_IDLE;
        return;
    }

    // Beep every second with countdown urgency
    if (millis() - lastBeepTime >= 1000) {
        lastBeepTime = millis();
        uint8_t remaining = (COUNTDOWN_DURATION_MS - elapsed) / 1000 + 1;
        Serial.print(F("[COUNTDOWN] Abort window: "));
        Serial.print(remaining);
        Serial.println(F(" seconds remaining..."));
        buzzer.playCountdownBeep(remaining);
    }

    // Check if grace period expired
    if (elapsed >= COUNTDOWN_DURATION_MS) {
        Serial.println(F("[DISPATCH] Grace period expired. Engaging emergency dispatch!"));
        buzzer.playTriggerChirp();
        currentState = STATE_SEND_SMS;
    }
}

void handleSendSms() {
    Serial.println(F("[SMS] Dispatching backup emergency SMS alert..."));
    digitalWrite(PIN_LED_RED, HIGH);

    char message[140];
    snprintf(message, sizeof(message),
             "EMERGENCY ALERT: %s requested at %s. Audio call dispatching now.",
             getEmergencyName(activeEmergency), PREMISES_ADDRESS);

    if (sim800l.sendSMS(BACKUP_SMS_NUMBER, message)) {
        Serial.println(F("[SMS] Backup SMS successfully sent."));
    } else {
        Serial.println(F("[SMS] Warning: SMS transmission failed or queued."));
    }

    dialRetryCount = 0;
    currentState = STATE_DIAL_CALL;
}

void handleDialCall() {
    const char* targetNumber = getEmergencyNumber(activeEmergency);
    Serial.print(F("[CALL] Dialing "));
    Serial.print(getEmergencyName(activeEmergency));
    Serial.print(F(" at "));
    Serial.print(targetNumber);
    Serial.print(F(" (Attempt "));
    Serial.print(dialRetryCount + 1);
    Serial.print(F("/"));
    Serial.print(MAX_DIAL_RETRIES);
    Serial.println(F(")..."));

    if (sim800l.dial(targetNumber)) {
        Serial.println(F("[CALL] Dial command sent. Monitoring call state..."));
        stateTimer = millis(); // Track ringing timeout
        currentState = STATE_CALL_WAIT_ANSWER;
    } else {
        Serial.println(F("[CALL] Dial command failed."));
        currentState = STATE_CALL_RETRY;
    }
}

void handleWaitAnswer() {
    // Check if user aborts via Cancel button
    if (checkButtonPressed(btnCancel)) {
        Serial.println(F("[ABORT] Cancel button pressed. Terminating call."));
        sim800l.hangUp();
        buzzer.playCancelChirp();
        activeEmergency = EMERGENCY_NONE;
        currentState = STATE_IDLE;
        return;
    }

    // Ringing timeout check
    if (millis() - stateTimer >= DIAL_TIMEOUT_MS) {
        Serial.println(F("[CALL] Ringing timeout (no answer after 45s)."));
        sim800l.hangUp();
        currentState = STATE_CALL_RETRY;
        return;
    }

    // Poll call state
    SimCallStatus status = sim800l.pollCallStatus();

    switch (status) {
        case SIM_CALL_ACTIVE:
            Serial.println(F("[CALL] >>> CALL ANSWERED AND CONNECTED <<<"));
            buzzer.playConnectedChirp();

            // Short pause to allow dispatcher introductory words
            delay(1500);

            audioRepeatCount = 0;
            isAudioCurrentlyPlaying = false;
            currentState = STATE_PLAY_AUDIO;
            break;

        case SIM_CALL_BUSY:
            Serial.println(F("[CALL] Remote line is BUSY."));
            sim800l.hangUp();
            currentState = STATE_CALL_RETRY;
            break;

        case SIM_CALL_NO_ANSWER:
            Serial.println(F("[CALL] Remote party did not answer."));
            sim800l.hangUp();
            currentState = STATE_CALL_RETRY;
            break;

        case SIM_CALL_NO_CARRIER:
            Serial.println(F("[CALL] Carrier disconnected or call dropped."));
            currentState = STATE_CALL_RETRY;
            break;

        case SIM_CALL_DIALING:
        case SIM_CALL_ALERTING:
        case SIM_CALL_IDLE:
        default:
            // Call still ringing
            break;
    }
}

void handlePlayAudio() {
    // Check if user aborts via Cancel button
    if (checkButtonPressed(btnCancel)) {
        Serial.println(F("[ABORT] Cancel button pressed during playback. Ending call."));
        dfPlayer.stop();
        sim800l.hangUp();
        buzzer.playCancelChirp();
        activeEmergency = EMERGENCY_NONE;
        currentState = STATE_IDLE;
        return;
    }

    // Monitor for remote party hang-up
    SimCallStatus status = sim800l.pollCallStatus();
    if (status == SIM_CALL_NO_CARRIER || status == SIM_CALL_IDLE) {
        Serial.println(F("[CALL] Remote dispatcher ended the call."));
        dfPlayer.stop();
        activeEmergency = EMERGENCY_NONE;
        currentState = STATE_IDLE;
        return;
    }

    // If audio has not been started for this repetition
    if (!isAudioCurrentlyPlaying) {
        uint16_t track = getEmergencyTrack(activeEmergency);
        Serial.print(F("[AUDIO] Playing voice payload track #"));
        Serial.print(track);
        Serial.print(F(" (Repetition "));
        Serial.print(audioRepeatCount + 1);
        Serial.print(F("/"));
        Serial.print(AUDIO_REPEAT_COUNT);
        Serial.println(F(")..."));

        dfPlayer.playMp3Folder(track);
        delay(300); // Small guard for DFPlayer BUSY pin to assert LOW
        isAudioCurrentlyPlaying = true;
        return;
    }

    // Check hardware BUSY pin for completion
    if (!dfPlayer.isPlaying()) {
        Serial.println(F("[AUDIO] Track repetition finished."));
        audioRepeatCount++;
        isAudioCurrentlyPlaying = false;

        if (audioRepeatCount < AUDIO_REPEAT_COUNT) {
            Serial.println(F("[AUDIO] Pausing 2 seconds before next repeat..."));
            delay(AUDIO_PAUSE_MS);
        } else {
            Serial.println(F("[CALL] All repetitions finished. Gracefully hanging up call."));
            delay(1000);
            sim800l.hangUp();
            buzzer.playTriggerChirp();
            activeEmergency = EMERGENCY_NONE;
            currentState = STATE_IDLE;
        }
    }
}

void handleCallRetry() {
    dialRetryCount++;
    if (dialRetryCount < MAX_DIAL_RETRIES) {
        Serial.print(F("[RETRY] Waiting 3 seconds before attempt "));
        Serial.print(dialRetryCount + 1);
        Serial.println(F("..."));
        delay(RETRY_DELAY_MS);
        currentState = STATE_DIAL_CALL;
    } else {
        Serial.println(F("[ALERT] Max dial retries exceeded! Emergency call unanswered."));
        buzzer.playErrorTone();
        activeEmergency = EMERGENCY_NONE;
        currentState = STATE_IDLE;
    }
}

// ==============================================================================
// HELPER FUNCTIONS
// ==============================================================================

bool checkButtonPressed(Button& btn) {
    bool reading = digitalRead(btn.pin);

    if (reading != btn.lastState) {
        btn.lastDebounceTime = millis();
    }

    if ((millis() - btn.lastDebounceTime) > BUTTON_DEBOUNCE_MS) {
        if (reading != btn.currentState) {
            btn.currentState = reading;
            // Button pressed (Active LOW)
            if (btn.currentState == LOW) {
                btn.lastState = reading;
                return true;
            }
        }
    }

    btn.lastState = reading;
    return false;
}

void updateLeds() {
    unsigned long now = millis();

    switch (currentState) {
        case STATE_IDLE:
            // Solid green, red off
            digitalWrite(PIN_LED_GREEN, HIGH);
            digitalWrite(PIN_LED_RED, LOW);
            break;

        case STATE_COUNTDOWN:
            // Fast alternating blink
            if (now - lastBlinkTime >= 150) {
                lastBlinkTime = now;
                greenLedState = !greenLedState;
                digitalWrite(PIN_LED_GREEN, greenLedState);
                digitalWrite(PIN_LED_RED, !greenLedState);
            }
            break;

        case STATE_SEND_SMS:
        case STATE_DIAL_CALL:
        case STATE_CALL_WAIT_ANSWER:
        case STATE_PLAY_AUDIO:
        case STATE_CALL_RETRY:
            // Red fast strobe during active emergency call
            if (now - lastBlinkTime >= 250) {
                lastBlinkTime = now;
                greenLedState = !greenLedState;
                digitalWrite(PIN_LED_RED, greenLedState);
                digitalWrite(PIN_LED_GREEN, LOW);
            }
            break;

        case STATE_FAULT:
            // Red slow heartbeat flash
            if (now - lastBlinkTime >= 600) {
                lastBlinkTime = now;
                greenLedState = !greenLedState;
                digitalWrite(PIN_LED_RED, greenLedState);
                digitalWrite(PIN_LED_GREEN, LOW);
            }
            break;

        case STATE_INIT:
        default:
            break;
    }
}

const char* getEmergencyName(EmergencyType type) {
    switch (type) {
        case EMERGENCY_POLICE:  return "POLICE";
        case EMERGENCY_FIRE:    return "FIRE";
        case EMERGENCY_MEDICAL: return "MEDICAL";
        default:                return "UNKNOWN";
    }
}

const char* getEmergencyNumber(EmergencyType type) {
    switch (type) {
        case EMERGENCY_POLICE:  return POLICE_PHONE_NUMBER;
        case EMERGENCY_FIRE:    return FIRE_PHONE_NUMBER;
        case EMERGENCY_MEDICAL: return MEDICAL_PHONE_NUMBER;
        default:                return "911";
    }
}

uint16_t getEmergencyTrack(EmergencyType type) {
    switch (type) {
        case EMERGENCY_POLICE:  return TRACK_POLICE;
        case EMERGENCY_FIRE:    return TRACK_FIRE;
        case EMERGENCY_MEDICAL: return TRACK_MEDICAL;
        default:                return 1;
    }
}