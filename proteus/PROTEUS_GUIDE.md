# Proteus Professional Simulation Guide

This folder contains pre-compiled firmware binaries and the complete schematic configuration for simulating the **Home Emergency Distress System** in **Proteus Professional (ISIS)**.

---

## 1. Compiled Firmware Files

| File | Purpose | When to Use in Proteus |
|---|---|---|
| **[firmware_simulation.hex](file:///home/shadhin/Documents/PlatformIO/Projects/Home%20Emergency/proteus/firmware_simulation.hex)** | **Recommended for Proteus** | Simulates GSM network registration, SMS dispatch, dispatcher call answering, and MP3 playback timing directly in software. Ideal for testing buttons, buzzer, LEDs, and logic flow without external hardware models. |
| **[firmware_production.hex](file:///home/shadhin/Documents/PlatformIO/Projects/Home%20Emergency/proteus/firmware_production.hex)** | **Hardware Build** | Real GSM & DFPlayer build. Use if you connect Proteus via `COMPIM` to a physical SIM800L or AT command modem emulator. |
| **[firmware_simulation.elf](file:///home/shadhin/Documents/PlatformIO/Projects/Home%20Emergency/proteus/firmware_simulation.elf)** | Debug Symbols | For source-level debugging inside Proteus VSM. |

---

## 2. Proteus Components List (Pick Devices)

Search and add the following devices from the Proteus Library (`P` button):

| Device Keyword | Device Name in Proteus | Quantity | Description / Connection |
|---|---|---|---|
| **`ARDUINO NANO`** or **`ATMEGA328P`** | Arduino Nano V3.0 (or ATmega328P) | 1 | Core Controller |
| **`BUTTON`** | Push Button (Active) | 4 | Momentary switches for emergency triggers |
| **`SOUNDER`** or **`BUZZER`** | Piezo Sounder / Buzzer | 1 | Audible alert on pin D7 |
| **`LED-GREEN`** | Animated Green LED | 1 | System Armed indicator on pin D12 |
| **`LED-RED`** | Animated Red LED | 1 | Emergency / Alert indicator on pin D13 |
| **`RES`** | 220Ω Resistors | 2 | Current limiting for LEDs |
| **`VIRTUAL TERMINAL`** | Virtual Instruments -> Virtual Terminal | 1 | Serial Monitor (115200 baud) |
| **`GROUND`** | Terminals Mode -> GROUND | Common | Common 0V reference |

---

## 3. Schematic Wiring Diagram in Proteus

### 3.1 Arduino Nano Pin Connections

```
                     ┌───────────────────────┐
                     │     ARDUINO NANO      │
                     │                       │
      [ Virtual RX ] <── TX (D1)      RX (D0) <── [ Virtual TX ]
                     │                       │
 [ Police Button ] ────> D2              D13 ───> [ 220Ω ] ───> [ LED-RED (Alert) ] ───> GND
   [ Fire Button ] ────> D3              D12 ───> [ 220Ω ] ───> [ LED-GREEN (Armed) ] ─> GND
[ Medical Button ] ────> D4                  │
 [ Cancel Button ] ────> D5              D7  ───> [ Buzzer (+) ] ───> GND
                     │                       │
                     └───────────────────────┘
```

### 3.2 Pin-by-Pin Wiring Table

| Arduino Nano Pin | Connected Device | Wiring Instructions |
|---|---|---|
| **D0 (RX)** | Virtual Terminal `TXD` | Connect Nano RX (pin 0) to Virtual Terminal `TXD` |
| **D1 (TX)** | Virtual Terminal `RXD` | Connect Nano TX (pin 1) to Virtual Terminal `RXD` |
| **D2** | Police Button | Connect one terminal of `BUTTON` to D2, other terminal to `GROUND` |
| **D3** | Fire Button | Connect one terminal of `BUTTON` to D3, other terminal to `GROUND` |
| **D4** | Medical Button | Connect one terminal of `BUTTON` to D4, other terminal to `GROUND` |
| **D5** | Cancel / Reset Button | Connect one terminal of `BUTTON` to D5, other terminal to `GROUND` |
| **D7** | Buzzer / Sounder | Connect Sounder pin 2 (+) to D7; pin 1 (-) to `GROUND` |
| **D12** | Green LED | Connect D12 to Resistor (220Ω), resistor to LED Anode, Cathode to `GROUND` |
| **D13** | Red LED | Connect D13 to Resistor (220Ω), resistor to LED Anode, Cathode to `GROUND` |
| **GND** | Power Rail | Connect to `GROUND` terminal |

> [!NOTE]
> All buttons use the internal Arduino `INPUT_PULLUP` resistor. No external pull-up resistors are required in Proteus; simply wire the other side of each button directly to `GROUND`.

---

## 4. How to Load the Firmware in Proteus

1. Place the **Arduino Nano** (or **ATmega328P**) component on your ISIS workspace.
2. **Double-click** (or Right-click -> *Edit Properties*) on the Arduino Nano / ATmega328P.
3. In the properties dialogue:
   - Find the **Program File** field.
   - Click the **Browse (folder icon)** button.
   - Select the file:
     ```
     <Your_Project_Path>/proteus/firmware_simulation.hex
     ```
   - If using a standalone **ATmega328P** component:
     - **Clock Frequency**: Change to `16MHz` (or `16000000`).
     - **CKSEL Fuses**: Set to `(1111) Ext. Crystal 8.0- MHz`.
4. Configure the **Virtual Terminal**:
   - Double-click the Virtual Terminal instrument.
   - Set **Baud Rate** to `115200`.
   - Set **Data Bits** to `8`.
   - Set **Stop Bits** to `1`.
   - Set **Parity** to `NONE`.
5. Click the **Play (Run Simulation)** button in the bottom-left corner of Proteus.

---

## 5. What to Expect During Simulation

1. **System Boot**:
   - The Virtual Terminal will pop up displaying:
     ```
     ==============================================
      Standalone Home Emergency Dispatch System    
     ==============================================
     [AUDIO] Initializing DFPlayer Mini... OK
     [GSM] Initializing SIM800L module... OK
     [SIMULATION] SIM800L Virtual Cellular Modem initialized.
     [GSM] Checking SIM card status... READY
     [GSM] Waiting for cellular network registration... REGISTERED (CSQ: 26 / 31)
     [SYSTEM] Armed and ready for emergency distress input.
     ```
   - **Green LED** lights up solid (Armed state).
2. **Emergency Trigger & 5s Grace Period**:
   - Click the **Police Button** (D2), **Fire Button** (D3), or **Medical Button** (D4).
   - The buzzer emits audible warning beeps (5, 4, 3, 2, 1 seconds).
   - The Green and Red LEDs flash alternately.
3. **Cancel Test (False Alarm Abort)**:
   - If you click the **Cancel Button** (D5) during the 5-second countdown, the system aborts immediately with a descending chirp and returns to the Armed state without sending SMS or placing calls.
4. **Automated Dispatch Execution**:
   - Allow the 5-second countdown to expire.
   - **Backup SMS** is dispatched and formatted in the Virtual Terminal with location and emergency type.
   - **Dialing begins**: Red LED fast-strobes.
   - After 3 seconds of ringing, the simulated dispatcher answers the call.
   - Buzzer plays the call-connected tone.
   - DFPlayer voice message plays for 4 seconds, pauses for 2 seconds, and repeats 3 times.
   - The system automatically hangs up (`ATH`) and returns to the Armed state.
5. **Mid-Call Abort**:
   - Clicking **Cancel** (D5) while the call is connected immediately terminates the call (`ATH`) and silences playback.

