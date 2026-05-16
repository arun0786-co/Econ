# SYSTEM OPERATION & DEBUGGING GUIDE

## Understanding the Data Flow

### Cycle 1: Robot Collects and Sends Data (Every 1.5 seconds)

```
ROBOT SIDE                           ROOM SIDE
─────────────────────────────────    ─────────────────────────────────

[1] Read Sensors
    ├─ DHT11: 28.5°C, 65%
    ├─ Ultrasonic: 45 cm away
    ├─ PIR Motion: Detected
    └─ Gas: 1200 ADC

    ↓ (calculate)

[2] Check Navigation
    ├─ Distance 45 cm > 20 cm?
    └─ YES → Move Forward

    ↓ (execute)

[3] Send via ESP-NOW
    └─ Transmit packet                [Receive packet]
                                      ↓
                                   [4] Store data
                                       └─ temp=28.5, motion=1, etc.
```

### Cycle 2: Room Makes Decisions (Every 2 seconds)

```
ROOM SIDE
─────────────────────────────────

[1] Check Light Rule
    └─ motion == HIGH? → Turn ON

[2] Check Fan Rule
    └─ temperature > 30°C? → Fan OFF (28.5 is OK)

[3] Check AC Rule
    └─ temperature > 35°C? → AC OFF

[4] Check Window Rule
    └─ temperature < 30°C? → Window OPEN

[5] Check Safety
    └─ gas_level > 2000? → Alert OFF (1200 is safe)

[6] Send Telegram (if needed)
    └─ (No alerts this cycle)
```

## Real Console Output Examples

### Robot Normal Operation

```
================================
🤖 ROBOT ESP32 STARTING UP
================================
✓ All systems initialized successfully
✓ Robot ready for autonomous operation

📡 Sent - Temp: 26.3°C | Dist: 120 cm
📡 Sent - Temp: 26.4°C | Dist: 118 cm
📡 Sent - Temp: 26.3°C | Dist: 89 cm
📡 Sent - Temp: 26.3°C | Dist: 35 cm    ← Getting closer to obstacle
📡 Sent - Temp: 26.3°C | Dist: 18 cm    ← OBSTACLE DETECTED! (< 20cm)
📡 Sent - Temp: 26.3°C | Dist: 25 cm    ← Turning right
📡 Sent - Temp: 26.3°C | Dist: 65 cm    ← Obstacle avoided
📡 Sent - Temp: 26.3°C | Dist: 120 cm   ← Back to normal
```

### Room Normal Operation (No Alerts)

```
================================
🏠 SMART ROOM CONTROLLER STARTING
================================
Connecting to WiFi: MyNetwork...
✓ WiFi connected!
IP Address: 192.168.1.45

✓ ESP-NOW Ready - Waiting for robot data...

✓ Received data from Robot - Temp: 26.3°C
✓ Received data from Robot - Temp: 26.4°C
✓ Received data from Robot - Temp: 26.3°C
✓ Received data from Robot - Temp: 26.3°C
✓ Received data from Robot - Temp: 26.3°C
```

### Room with Temperature Alert (Fan/AC Trigger)

```
✓ Received data from Robot - Temp: 31.2°C  ← Temperature rising
✓ Received data from Robot - Temp: 32.5°C  ← FAN TURNS ON
✓ Received data from Robot - Temp: 33.8°C  ← Still rising
✓ Received data from Robot - Temp: 35.1°C  ← AC TURNS ON
✓ Received data from Robot - Temp: 34.9°C  ← Cooling effect
✓ Received data from Robot - Temp: 33.2°C  ← Temperature dropping
```

### Room with Emergency Gas Alert

```
✓ Received data from Robot - Temp: 28.5°C
✓ Received data from Robot - Temp: 28.6°C
✓ Received data from Robot - Gas Level: 2150  ← HIGH!
⚠️  Alert sent to Telegram                    ← First alert sent
✓ Received data from Robot - Temp: 28.7°C
✓ Received data from Robot - Gas Level: 2300
📡 (No alert - waiting 10 seconds before next)
✓ Received data from Robot - Temp: 28.9°C
✓ Received data from Robot - Gas Level: 2250
⚠️  Alert sent to Telegram                    ← Second alert after 10s
```

## Automation Rule Truth Table

### Lighting Control

| Motion Detected | Lights Status |
|-----------------|---------------|
| YES (HIGH) | ON (GPIO26 = HIGH) |
| NO (LOW) | OFF (GPIO26 = LOW) |

### Temperature Control - Fan

| Temperature | Fan Status | Reason |
|-------------|-----------|--------|
| < 30°C | OFF | Below threshold |
| 30°C | OFF | At threshold, not above |
| 30.1°C | ON | Above threshold |
| > 30°C | ON | Keeps running |

### Temperature Control - AC

| Temperature | AC Status | Reason |
|-------------|-----------|--------|
| < 35°C | OFF | Below threshold |
| 35°C | OFF | At threshold, not above |
| 35.1°C | ON | Above threshold |
| > 35°C | ON | Keeps running |

### Window Control

| Temperature | Window Position | Servo Angle |
|-------------|-----------------|-------------|
| < 30°C | OPEN | 90° |
| = 30°C | OPEN | 90° |
| > 30°C | CLOSED | 0° |

### Gas Safety Alert

| Gas ADC Reading | Alert Status | Telegram | Repeat |
|-----------------|--------------|----------|--------|
| < 2000 | SAFE | No | N/A |
| = 2000 | SAFE | No | N/A |
| > 2000 | ALERT | YES | Every 10s |
| >> 2000 | CRITICAL | YES | Every 10s |

## Debugging Workflow

### Issue: "No data received in room console"

```
Check 1: Robot Serial Output
├─ Shows "✓ Robot ESP32 Started"? 
│  └─ YES → Go to Check 2
│  └─ NO → Robot not running or upload failed
│
Check 2: Robot Shows "✓ ESP-NOW Ready"?
│  ├─ YES → Go to Check 3
│  └─ NO → ESP-NOW initialization failed
│     └─ Solution: Check power supply, try USB reset
│
Check 3: Robot Shows "📡 Sent - Temp: X.X°C"?
│  ├─ YES → Data is being sent! Go to Check 4
│  └─ NO → Sensor reading failed
│     ├─ DHT11 not responding? Check power
│     ├─ Ultrasonic not working? Check TRIG/ECHO pins
│     └─ Try restarting robot
│
Check 4: Room MAC Address Correct?
│  ├─ YES → Go to Check 5
│  └─ NO → Update MAC in room.ino line 47-48
│     └─ Solution: Find robot MAC, update room code
│
Check 5: Both devices on same WiFi channel?
│  ├─ YES → Go to Check 6
│  └─ NO → ESP-NOW uses WiFi channel
│     └─ Solution: Reset both devices
│
Check 6: Check Room Serial Output
│  ├─ Shows "✓ ESP-NOW Ready"? 
│  │  └─ NO → Room ESP-NOW not initialized
│  │     └─ Try: Power cycle room device
│  └─ Still no data after all checks?
│     └─ Solution: Try updating both devices' firmware
```

### Issue: "Lights not responding to motion"

```
Check 1: Robot Motion Sensor
├─ Does robot console show motion detection?
│  ├─ YES (motion_detected = 1) → Go to Check 2
│  └─ NO → Robot motion sensor issue
│     └─ Solution: Check PIR sensor power and GPIO19 connection
│
Check 2: Room Receives Motion Data
├─ Does room show "Temp: X, Motion: 1" in incoming data?
│  ├─ YES → Go to Check 3
│  └─ NO → Data not transmitting properly
│     └─ Solution: Check ESP-NOW connection
│
Check 3: Room Automation Logic
├─ Code should set LIGHT_PIN HIGH when motion=1
│  ├─ Test: Manually set LIGHT_PIN = HIGH in code
│  ├─ Do lights turn on? 
│  │  ├─ YES → Motion logic might not be running
│  │  │   └─ Solution: Check main loop executes
│  │  └─ NO → Light hardware issue
│  │     └─ Check GPIO26 connections, relay, bulb
```

### Issue: "Temperature control not working"

```
Check 1: Temperature Reading Valid
├─ Does robot show actual temperature (not -999)?
│  ├─ YES → Go to Check 2
│  └─ NO → DHT11 sensor issue
│     ├─ Check power supply (3-5V)
│     ├─ Check DATA pin on GPIO4
│     └─ Try different sensor
│
Check 2: Room Receives Temperature
├─ Does room console show correct temp value?
│  ├─ YES → Go to Check 3
│  └─ NO → ESP-NOW transmission issue
│
Check 3: Automation Logic Running
├─ Test by heating sensor with warm air
│  ├─ When > 30°C, does fan turn on?
│  │  ├─ YES → Check AC trigger at 35°C
│  │  └─ NO → Check GPIO25 connections (fan relay)
│     └─ Verify fan power supply, relay module
│
Check 4: Threshold Values
├─ Try lowering thresholds for testing:
│  ├─ Set TEMP_FAN_THRESHOLD = 25
│  ├─ Set TEMP_AC_THRESHOLD = 26
│  └─ Recompile and upload
└─ Does fan now turn on at lower temp?
   ├─ YES → Thresholds were above actual room temp
   └─ NO → GPIO or relay hardware issue
```

## Performance Monitoring

### Expected Data Rates

| Device | Data | Frequency | Bandwidth |
|--------|------|-----------|-----------|
| Robot | Sensor readings | Every 1.5s | ~11 bytes/1.5s ≈ 73 bps |
| Room | Decision execution | Every 2.0s | No transmission |
| WiFi | Telegram alerts | When needed | ~300 bytes/alert |

### Power Consumption Estimates

| Device | Standby | Active | Duration |
|--------|---------|--------|----------|
| Robot | 150mA | 350mA | 2-3 hours (2000mAh) |
| Room | 200mA | 500mA | Continuous (mains) |

### Timing Accuracy

- Robot cycle: 1500ms ± 50ms (acceptable variance)
- Room cycle: 2000ms ± 100ms (not critical)
- Telegram send: 2-5 seconds (depends on WiFi)

## LED Status Indicators (if added)

### Robot

```
🟢 GREEN (steady) → Normal operation, sensors OK
🟡 YELLOW (slow blink 1/sec) → Sensor reading error
🔴 RED (steady) → Motor control failure
⚫ OFF → Power issue
```

### Room

```
🟢 GREEN (steady) → WiFi connected, receiving data
🟡 YELLOW (blink) → WiFi connecting
🔴 RED (steady) → WiFi failed
⚫ OFF → Power issue
```

## Testing Checklist

- [ ] Serial output shows sensor values (not NaN or -999)
- [ ] Ultrasonic detects obstacles (distance < 400 cm)
- [ ] Robot moves forward when path clear
- [ ] Robot stops and turns when obstacle < 20cm
- [ ] Room receives all sensor data (no drops)
- [ ] Lights respond to motion within 1 second
- [ ] Fan turns on above 30°C
- [ ] AC turns on above 35°C
- [ ] Window servo moves (0° to 90°)
- [ ] Telegram receives startup message
- [ ] Telegram receives gas alert when testing
- [ ] All components power down safely
- [ ] System resumes after power cycle

## Advanced Debugging

### Enable Debug Mode (Modify Code)

In robot code, add after Serial.println statements:

```cpp
// Robot - add detailed logging
Serial.print("DEBUG - Temperature raw: ");
Serial.println(analogRead(GAS_SENSOR_PIN));
Serial.print("DEBUG - Motor state: ");
Serial.println(sensor_readings.distance_to_obstacle);
```

In room code:

```cpp
// Room - log all decisions
Serial.print("DEBUG - Light: ");
Serial.println(incoming_sensor_data.motion_detected);
Serial.print("DEBUG - Fan trigger temp: ");
Serial.println(incoming_sensor_data.temperature);
```

### Use Serial Plotter

Arduino IDE → Tools → Serial Plotter

Shows real-time graphs of sensor values for easy trend spotting.

## Common Timing Issues

### Problem: "Data arrives 5+ seconds late"

**Likely Causes:**
1. Robot transmission interval too long (> 2000ms)
2. WiFi interference affecting ESP-NOW
3. Room loop delay too large

**Solution:**
```cpp
// Reduce intervals:
#define SENSOR_READ_INTERVAL 1000  // Instead of 1500
#define AUTOMATION_CHECK_INTERVAL 1000  // Instead of 2000
```

### Problem: "Random data loss every few minutes"

**Likely Causes:**
1. Buffer overflow
2. WiFi scanning interfering with ESP-NOW
3. Power supply fluctuation

**Solution:**
```cpp
// Reduce data transmission size:
// Keep only critical fields in struct
// Remove debug messages during operation
```

## Next Level Debugging

For deep issues, log to SD card:

```cpp
// Add to libraries: SD.h
#include <SD.h>

// Log sensor data to file every 10 seconds
if (millis() % 10000 == 0) {
  File dataFile = SD.open("sensor_log.csv", FILE_WRITE);
  dataFile.print(millis());
  dataFile.print(",");
  dataFile.println(sensor_readings.temperature);
  dataFile.close();
}
```

Then download logs to analyze offline!
