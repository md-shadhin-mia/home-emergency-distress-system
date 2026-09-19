#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ==============================================================================
// SIMULATION MODE SWITCH
// ==============================================================================
// Set to 1 (or build with -DSIMULATION_MODE=1) to simulate SIM800L and DFPlayer
// responses in Wokwi/Proteus without requiring physical GSM/audio modules.
// Set to 0 when flashing to real hardware with physical SIM800L and DFPlayer.
#ifndef SIMULATION_MODE
#define SIMULATION_MODE         0
#endif

// ==============================================================================
// EMERGENCY CONTACTS & PREMISES CONFIGURATION
// ==============================================================================
// Replace these with your local emergency dispatch or designated family/caregiver numbers
#define POLICE_PHONE_NUMBER     "911"
#define FIRE_PHONE_NUMBER       "911"
#define MEDICAL_PHONE_NUMBER    "911"

// Backup SMS recipient (include international country code e.g. "+1XXXXXXXXXX")
#define BACKUP_SMS_NUMBER       "+10000000000"

// Physical premises location text injected into the backup SMS
#define PREMISES_ADDRESS        "123 Main St, Apt 4B, Springfield"

// ==============================================================================
// HARDWARE PIN ASSIGNMENTS (ARDUINO NANO)
// ==============================================================================
// Emergency Momentary Push Buttons (Active LOW with internal INPUT_PULLUP)
#define PIN_BTN_POLICE          2
#define PIN_BTN_FIRE            3
#define PIN_BTN_MEDICAL         4
#define PIN_BTN_CANCEL          5

// DFPlayer Mini Hardware Status & Serial Pins
#define PIN_DF_BUSY             6       // DFPlayer BUSY pin (LOW = Playing, HIGH = Idle)
#define PIN_DF_TX               10      // Arduino TX (D10) -> DFPlayer RX (via 1k resistor)
#define PIN_DF_RX               11      // Arduino RX (D11) <- DFPlayer TX

// Audible Buzzer
#define PIN_BUZZER              7       // Piezo buzzer (+) pin

// SIM800L GSM Module Pins
// Note: AltSoftSerial uses dedicated hardware pins on ATmega328P:
// Arduino D8 = AltSoftSerial RX (Connects to SIM800L TXD)
// Arduino D9 = AltSoftSerial TX (Connects to SIM800L RXD via 1k/2k voltage divider)
#define PIN_SIM_RST             A0      // Hardware Reset pin to power cycle/reset SIM800L

// Status LEDs
#define PIN_LED_GREEN           12      // Armed & GSM Network Registered
#define PIN_LED_RED             13      // Fault / No Signal / Emergency in Progress

// ==============================================================================
// OPERATIONAL & TIMING CONSTANTS
// ==============================================================================
#define COUNTDOWN_DURATION_MS   5000UL  // Abort grace period window in milliseconds
#define AUDIO_REPEAT_COUNT      3       // Number of times voice payload is repeated
#define AUDIO_PAUSE_MS          2000UL  // Pause between voice payload repetitions
#define MAX_DIAL_RETRIES        3       // Maximum call attempts if busy or unanswered
#define DIAL_TIMEOUT_MS         45000UL // Ringing timeout before treating as unanswered
#define RETRY_DELAY_MS          3000UL  // Wait time between failed call retries
#define BUTTON_DEBOUNCE_MS      40UL    // Debounce duration for physical switches

// Audio Tuning Constants
#define DFPLAYER_VOLUME         20      // Volume level (0 - 30). 20 is optimal for low distortion
#define SIM800L_MIC_GAIN        6       // Microphone channel gain (0 - 15) via AT+CMIC=0,x

// SD Card Track IDs (/mp3/0001.mp3, etc.)
#define TRACK_POLICE            1
#define TRACK_FIRE              2
#define TRACK_MEDICAL           3

// Baud Rates
#define SERIAL_DEBUG_BAUD       9600    // USB Serial Monitor baud rate (9600 for Proteus Virtual Terminal default)
#define SIM800L_BAUD            9600    // SIM800L AltSoftSerial baud rate
#define DFPLAYER_BAUD           9600    // DFPlayer Mini SoftwareSerial baud rate

#endif // CONFIG_H

