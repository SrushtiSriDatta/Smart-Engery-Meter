# Intelligent Energy Meter System for Theft and Tamper Prevention

**ECMI 2026 Conference Paper** | Department of Electronics & Communication Engineering  


**Authors:** Srushti SriDatta, 
---

## Abstract

This project presents an IoT-enabled intelligent energy metering system designed to measure electrical parameters in real time and prevent electricity theft and physical tampering. The system integrates voltage and current sensing, physical tamper detection, automated protection using a relay and buzzer, GSM-based alerting, and cloud-based monitoring through ThingSpeak.

---

## Features

- Real-time AC voltage, current, and power measurement
- Overload detection with automatic relay trip and buzzer alert
- Physical tamper detection via Reed switch and Hall effect sensor
- GSM-based SMS alerts for theft, overload, and startup events
- IoT cloud monitoring via ThingSpeak dashboard
- LCD display showing live electrical parameters
- Startup SMS with initial readings after boot

---

## System Architecture

```
Power Supply
     │
     ▼
   ESP32 ──────────────────────────────────────────────────┐
     │                                                      │
     ├── ACS712 Current Sensor (Pin 34)                    │
     ├── ZMPT101B Voltage Sensor (Pin 32)                  │
     ├── Reed Switch (Pin 27)  ──── Tamper Detection       │
     ├── Hall Effect Sensor (Pin 14) ─ Tamper Detection    │
     ├── Relay Module (Pin 25) ──── Load Control           │
     ├── Buzzer (Pin 26)                                    │
     ├── LCD Display I2C (0x27, 16x2)                      │
     ├── SIM800L GSM Module (TX:16, RX:17)                 │
     └── WiFi ──────────────────────────────── ThingSpeak ─┘
```

---

## Hardware Components

| Component | Model | Purpose |
|-----------|-------|---------|
| Microcontroller | ESP32 | Core processing, WiFi, ADC |
| Current Sensor | ACS712 | AC current measurement |
| Voltage Sensor | ZMPT101B | AC voltage measurement |
| GSM Module | SIM800L | SMS alerts |
| Tamper Sensor 1 | Reed Switch | Enclosure open detection |
| Tamper Sensor 2 | Hall Effect Sensor | Magnetic interference detection |
| Display | 16x2 LCD (I2C) | Real-time parameter display |
| Protection | Relay Module | Automatic load disconnection |
| Alert | Buzzer | Local audible alarm |
| Cloud | ThingSpeak | IoT data visualization |

---

## Pin Configuration

| Pin | Component | Function |
|-----|-----------|----------|
| 34 | ACS712 | Current sensor analog input |
| 32 | ZMPT101B | Voltage sensor analog input |
| 25 | Relay | Load control output |
| 26 | Buzzer | Alert output |
| 27 | Reed Switch | Tamper input (INPUT_PULLUP) |
| 14 | Hall Effect | Tamper input (INPUT_PULLUP) |
| 16 | SIM800L TX | GSM serial RX |
| 17 | SIM800L RX | GSM serial TX |
| SDA/SCL | LCD I2C | Display (address 0x27) |

---

## Software & Libraries

- **Arduino IDE**
- `LiquidCrystal_I2C` — LCD control
- `WiFi.h` — ESP32 WiFi connectivity
- `ThingSpeak.h` — IoT cloud data upload

---

## Getting Started

### Prerequisites
- Arduino IDE with ESP32 board support installed
- ThingSpeak account with a channel configured (5 fields)
- SIM card inserted in SIM800L

### Setup

1. Clone the repository:
   ```bash
   git clone https://github.com/SrushtiSriDatta/Smart-Engery-Meter.git
   ```

2. Open `Smart Enegry meter.ino` in Arduino IDE

3. Update the configuration in the file:
   ```cpp
   const char* ssid     = "YOUR_WIFI_SSID";
   const char* password = "YOUR_WIFI_PASSWORD";

   unsigned long myChannelNumber = YOUR_CHANNEL_NUMBER;
   const char* myWriteAPIKey    = "YOUR_THINGSPEAK_API_KEY";

   // In sendSMS() function:
   Serial2.println("AT+CMGS=\"+91XXXXXXXXXX\"");  // your phone number
   ```

4. Select board: **ESP32 Dev Module**

5. Upload to your ESP32

---

## ThingSpeak Channel Fields

| Field | Data |
|-------|------|
| Field 1 | Current (A) |
| Field 2 | Voltage (V) |
| Field 3 | Power (W) |
| Field 4 | Overload Status (0/1) |
| Field 5 | Theft Detected (0/1) |

---

## System Behaviour

### Normal Operation
- LCD displays live current, voltage, and power values
- Data uploaded to ThingSpeak every 20 seconds
- Relay remains ON (load connected)

### Overload Detection (> 200W)
- Relay trips permanently (load disconnected)
- Buzzer activates for 2 seconds
- SMS alert sent: *"Alert: Load exceeded 200W. Power cut off permanently. Reset required."*
- LCD shows: `!! FAULT !! / RESET REQUIRED`

### Tamper Detection
- Reed switch or Hall effect sensor triggers on enclosure open / magnetic interference
- Relay trips permanently
- Buzzer activates for 2 seconds
- SMS alert sent: *"Theft Alert: Unauthorized access detected!"*
- LCD shows: `THEFT ALERT / Power Cut Off`

### Startup SMS
- Sent 5 seconds after boot with initial voltage, current, and power readings

---

## Experimental Results

| Scenario | System Response |
|----------|----------------|
| Normal operation | Stable readings on LCD and ThingSpeak, relay ON |
| Overload (>200W) | Relay tripped, buzzer activated, SMS sent |
| Enclosure tamper | Power cut, buzzer activated, SMS sent |
| Magnetic interference | Power cut, buzzer activated, SMS sent |

---

## Conference

This system was presented at the **ECMI 2026 Conference**:

> Dr. Raghavendra Y. M., Srushti SriDatta, Srusti Gopal, Tejashwini N,
> *"Intelligent Energy Meter System for Theft and Tamper Prevention"*,
> ECMI 2026, Department of ECE,  VTU Belagavi.

---

## License

ISC
