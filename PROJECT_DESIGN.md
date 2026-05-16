# Sustainable AI-Based Smart Home System - Architecture & Design

## System Overview

Our system comprises two intelligent ESP32 nodes working in harmony:
- **Robot Node**: Autonomous environmental monitor with navigation capabilities
- **Room Node**: Central controller managing home automation and alerts

## System Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                    ESP-NOW Wireless Network                      │
└─────────────────────────────────────────────────────────────────┘
                    ↑                           ↑
                    │                           │
            ┌───────┴─────────┐         ┌──────┴────────┐
            │                 │         │               │
    ┌───────▼─────────┐   ┌───▼─────────▼────────┐   ┌──▼─────────────────┐
    │  ROBOT ESP32    │   │   Room ESP32 Main    │   │  Cloud Services    │
    │  (Node A)       │   │   (Node B - Hub)     │   │  (Telegram)        │
    └───────┬─────────┘   └───┬──────────────────┘   └────────────────────┘
            │                 │
    ┌───────▼──────┐  ┌──────▼───────┐
    │ Sensors:     │  │ Actuators:   │
    │ • DHT11      │  │ • Lights     │
    │ • Ultrasonic │  │ • Fan        │
    │ • MQ Gas     │  │ • AC         │
    │ • PIR Motion │  │ • Servo      │
    └──────────────┘  └──────────────┘
```

## Data Flow Diagram

```
ROBOT SIDE (Continuous Cycle):
┌─────────────────────────────────────────┐
│  1. Read All Sensors                    │
│     └─> DHT (Temp/Humidity)            │
│     └─> Ultrasonic (Distance)          │
│     └─> MQ Gas (Air Quality)           │
│     └─> PIR (Motion Detection)         │
└────────────────┬────────────────────────┘
                 │
                 ▼
    ┌────────────────────────────┐
    │ 2. Process Navigation      │
    │    └─> Obstacle Check      │
    │    └─> Decision Making     │
    │    └─> Motor Control       │
    └────────────┬───────────────┘
                 │
                 ▼
    ┌────────────────────────────┐
    │ 3. Send Data via ESP-NOW   │
    │    └─> Format Struct       │
    │    └─> Transmit to Room    │
    └────────────────────────────┘
                 │
                 ▼
           Wait 1.5 Seconds
           Loop Again


ROOM SIDE (Event-Driven):
┌─────────────────────────────────────────┐
│  1. Receive Data from Robot (ESP-NOW)   │
│     └─> Callback Function Triggered     │
│     └─> Store in Buffer                 │
└────────────────┬────────────────────────┘
                 │
                 ▼
    ┌────────────────────────────────────┐
    │ 2. Check Automation Rules          │
    │    └─> Light (if Motion detected)  │
    │    └─> Fan (if Temp > 30°C)       │
    │    └─> AC (if Temp > 35°C)        │
    │    └─> Window (Temperature based)  │
    └────────────┬───────────────────────┘
                 │
                 ▼
    ┌────────────────────────────────────┐
    │ 3. Safety Monitoring               │
    │    └─> Gas Level Checking          │
    │    └─> Alert on Threshold         │
    └────────────┬───────────────────────┘
                 │
                 ▼
    ┌────────────────────────────────────┐
    │ 4. Send Telegram Alerts (if needed)│
    │    └─> Connect to WiFi             │
    │    └─> Send Alert Message          │
    │    └─> With Timeout Protection     │
    └────────────────────────────────────┘
```

## Communication Protocol

### ESP-NOW Message Structure
```
Message Format (11 bytes total):
┌──────────────┬──────────────┬──────────┬────────┬──────────┐
│ Temperature  │  Humidity    │ Gas Lvl  │ Motion │ Distance │
│  (4 bytes)   │  (4 bytes)   │ (2 bytes)│ (1 b)  │ (2 bytes)│
└──────────────┴──────────────┴──────────┴────────┴──────────┘
• Transmission Interval: 1.5 seconds
• Range: ~250 meters (line of sight)
• Frequency: 2.4 GHz (WiFi Channel 0)
```

## Sensor Specifications & Thresholds

| Sensor | Range | Purpose | Alert Threshold |
|--------|-------|---------|-----------------|
| DHT11 | 0-50°C | Environment monitoring | N/A |
| Ultrasonic | 2-400cm | Obstacle detection | < 20cm |
| MQ Gas | 0-1023 ADC | CO/Smoke detection | > 2000 ADC |
| PIR | Boolean | Motion sensing | Motion detected |

## Automation Thresholds

```
TEMPERATURE-BASED CONTROL:
├─ T < 30°C  → Window OPEN (90°)
├─ T ≥ 30°C  → Window CLOSED (0°)
├─ T > 30°C  → Fan ON
├─ T > 35°C  → AC ON
└─ Others    → OFF

MOTION-BASED CONTROL:
└─ Motion Detected → Lights ON (rest of cycle)
└─ No Motion       → Lights OFF

SAFETY CHECKS:
└─ Gas Level > 2000 → Emergency Alert (Telegram)
```

## Power Consumption Considerations

- Robot: Battery-powered, optimized for 2-3 hour runtime
- Room: Mains-powered hub
- Both devices minimize WiFi usage (ESP-NOW is very efficient)
- Transmission occurs every 1.5 seconds

## Security Notes

- WiFi credentials must be configured in Room ESP32
- Telegram Bot Token required for alerts
- No encryption on ESP-NOW (local network only)
- Recommend secured IoT network for production

## Future Enhancements

1. MQTT broker integration for remote monitoring
2. Cloud-based data logging
3. Machine learning for energy optimization
4. Voice control integration
5. Mobile app for monitoring
6. Battery status monitoring for robot
