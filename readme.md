# Sustainable AI-Based Smart Home and Autonomous Robot System

## Overview
This project implements a smart room automation system integrated with an autonomous robot for environmental monitoring and safety. The system focuses on energy efficiency, real-time automation, and carbon monoxide detection using ESP32 microcontrollers.

## Features
- Smart lighting, fan, AC, and window automation
- Autonomous robot navigation with obstacle avoidance
- Temperature, humidity, smoke, and CO monitoring
- ESP-NOW based ESP32-to-ESP32 communication
- Real-time Telegram alerts
- Sustainable and energy-efficient design

## Technologies Used
- ESP32
- ESP-NOW
- Arduino (Embedded C/C++)
- Telegram Bot API
- IoT Sensors (DHT11, MQ Gas Sensor, Ultrasonic)



## Hardware Setup Required

### Robot ESP32 Component List

| Component | Model | GPIO | Purpose |
|-----------|-------|------|---------|
| Ultrasonic Sensor | HC-SR04 | GPIO 5, 18 | Obstacle detection (20cm threshold) |
| Motion Sensor | HC-SR501 PIR | GPIO 19 | Motion detection for lights |
| Gas Sensor | MQ-X Series | GPIO 34 ADC | CO/Smoke detection (>2000 ADC alert) |
| Temperature/Humidity | DHT11 | GPIO 4 | Environment monitoring |
| Motor Driver | L298N | GPIO 13,12,14,27 | Robot movement control |
| Wheels/Motors | DC Motors | L298N output | Locomotion |
| Power Source | Li-Ion Battery | Vin | Robot power (2-3 hours) |

### Room ESP32 Component List

| Component | Model | GPIO | Purpose |
|-----------|-------|------|---------|
| LED/Lights | Any 5V | GPIO 26 | Motion-triggered lighting |
| Fan Motor | Any 5V | GPIO 25 | Temperature control (>30°C) |
| AC Relay | 5V Relay | GPIO 33 | AC activation (>35°C) |
| Servo Motor | 9g Servo | GPIO 14 | Window automation |
| Motion Sensor | HC-SR501 PIR | GPIO 27 | Room motion detection (optional) |
| Power Source | Mains/5V Supply | Vin | Room controller power |

## Code Configuration Steps

### Step 1: Update Robot MAC Address in room.ino

You need to find your robot ESP32's MAC address first:

**Option A: Check via Serial Monitor**
1. Upload a simple MAC address scanner to robot
2. Open Serial Monitor (baud rate: 115200)
3. Note the MAC address displayed

**Option B: Know the default from packaging**
- Some ESP32s have MAC printed on them

Once you have it, update `room.ino`:

```cpp
// Line 47-48 in room.ino
// Change this:
uint8_t room_esp32_mac[] = {0x24, 0x6F, 0x28, 0xAA, 0xBB, 0xCC};

// To your actual room ESP32 MAC address like:
uint8_t room_esp32_mac[] = {0x84, 0x0D, 0x8E, 0x12, 0x34, 0x56};
```

### Step 2: Configure WiFi in room.ino

Update WiFi credentials:

```cpp
// Line 22-23 in room.ino
const char *wifi_ssid = "YOUR_ACTUAL_WIFI_NAME";        // Change this!
const char *wifi_password = "YOUR_ACTUAL_PASSWORD";      // Change this!
```

### Step 3: Setup Telegram Bot (Optional but Recommended)

To receive emergency alerts on your phone:

1. **Open Telegram** and find **@BotFather**
2. **Create a new bot**:
   - Send `/newbot`
   - Choose a name (e.g., "SmartHomeBot")
   - Choose a username (e.g., "my_smarthome_bot")
   - Copy the **Bot Token** (long string with colons)

3. **Get your Chat ID**:
   - Find **@userinfobot** in Telegram
   - Send it any message
   - It will reply with your Chat ID (numbers)

4. **Update room.ino**:

```cpp
// Line 24-25 in room.ino
String telegram_bot_token = "YOUR_BOT_TOKEN_HERE";   // e.g., "123456789:ABCdefGHIjklmnoPQRstuvWXYZ"
String telegram_chat_id = "YOUR_CHAT_ID_HERE";        // e.g., "987654321"
```

### Step 4: Adjust Automation Thresholds (Optional)

The system has smart defaults, but customize based on your climate:

```cpp
// In room.ino - adjust these values:

#define TEMP_FAN_THRESHOLD 30           // Fan turns on above 30°C
#define TEMP_AC_THRESHOLD 35            // AC turns on above 35°C
#define TEMP_WINDOW_OPEN_THRESHOLD 30   // Window opens below 30°C
#define GAS_ALERT_LEVEL 2000            // Alert if gas > 2000 ADC
#define SENSOR_READ_INTERVAL 1500       // Robot sends data every 1.5s
```

## Uploading Code to ESP32

### Prerequisites

You need:
1. Arduino IDE installed
2. ESP32 board package installed
3. USB cables for both ESP32s
4. PySerial library (for board detection)

### Arduino IDE Setup

1. Open Arduino IDE
2. Go to **Preferences**
3. Add board manager URL: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
4. Go to **Boards Manager** and search "esp32"
5. Install "esp32 by Espressif Systems"

### Upload Robot Code

```
1. Select Board: "ESP32 Dev Module"
2. Select Port: (e.g., /dev/cu.usbserial-xxxxx)
3. Open: robot_esp32/robo.ino
4. Click Upload (Ctrl+U)
5. Open Serial Monitor (Ctrl+Shift+M)
6. Set Baud Rate to 115200
7. Verify it says "✓ Robot ESP32 Started"
```

### Upload Room Code

```
1. Disconnect robot, connect room ESP32
2. Select Port: (your room ESP32's port)
3. Open: room_esp32/room.ino
4. Make sure WiFi and Telegram credentials are set!
5. Click Upload (Ctrl+U)
6. Open Serial Monitor
7. Set Baud Rate to 115200
8. Verify WiFi connects and "✓ Smart Room Controller Starting"
9. Should receive Telegram message "✅ Smart Room System Started"
```

## Testing the System

### Serial Monitor Output

**Robot console should show:**
```
================================
🤖 ROBOT ESP32 STARTING UP
================================
✓ All systems initialized successfully
✓ Robot ready for autonomous operation

📡 Sent - Temp: 28.5°C | Dist: 45 cm
📡 Sent - Temp: 28.5°C | Dist: 45 cm
```

**Room console should show:**
```
================================
🏠 SMART ROOM CONTROLLER STARTING
================================
Connecting to WiFi: YOUR_WIFI_NAME...
✓ WiFi connected!
IP Address: 192.168.x.x
✓ ESP-NOW Ready - Waiting for robot data...
✓ All systems initialized successfully
✓ Alert sent to Telegram

✓ Received data from Robot - Temp: 28.5°C
✓ Received data from Robot - Temp: 28.5°C
```

### Functional Testing

1. **Test Lighting**
   - Move your hand near robot's motion sensor
   - Room lights should turn ON
   - Remove hand, lights should turn OFF after 1.5s

2. **Test Temperature Control**
   - Heat robot sensor with warm air
   - When > 30°C: Fan should turn ON
   - When > 35°C: AC should turn ON
   - When < 30°C: Window servo should move to open position

3. **Test Gas Alert**
   - If gas sensor reads > 2000 ADC
   - Should receive Telegram alert
   - Alert repeats every 10 seconds max

4. **Test Robot Navigation**
   - Place obstacle 15cm in front of robot
   - Robot should stop and turn right
   - When obstacle removed, should move forward

## Troubleshooting

### WiFi Won't Connect

**Problem:** Room ESP32 shows "⚠️ WiFi connection failed"

**Solution:**
- Check SSID is spelled correctly (case-sensitive)
- Check password is correct
- Ensure WiFi is 2.4GHz (ESP32 doesn't support 5GHz)
- Try resetting WiFi credentials

### Robot Doesn't Send Data

**Problem:** Room console doesn't show "Received data from Robot"

**Solution:**
- Check robot's MAC address matches in room.ino
- Verify both ESP32s are powered on
- Check serial output shows "✓ ESP-NOW Ready" on both
- Try uploading robot code again
- Restart both devices

### Telegram Alerts Not Arriving

**Problem:** No Telegram messages received

**Solution:**
- Verify WiFi is connected (room console shows IP address)
- Check Telegram bot token and chat ID are correct
- Test WiFi connection is working (room stays connected)
- Ensure gas sensor value exceeds 2000 threshold
- Check Telegram bot is still active (@BotFather)

### Ultrasonic Sensor Reading 999

**Problem:** Distance always shows 999

**Solution:**
- Check ultrasonic TRIG and ECHO pins are connected
- Verify connections are not loose
- Try in different environment (reflective surfaces help)
- Check sensor isn't damaged (should have 4 pins)
- Ensure trigger pulse and echo pin are working

### DHT11 Shows NaN

**Problem:** Temperature/humidity show NaN (not a number)

**Solution:**
- DHT11 has a specific data wire protocol
- Ensure DATA pin is GPIO 4 as configured
- Add 10kΩ pull-up resistor on DATA line
- Verify sensor is DHT11 (not DHT22 which needs different timing)
- Ensure sensor isn't wet or corroded

## Performance Optimization Tips

1. **Reduce Power Consumption**
   - Increase `SENSOR_READ_INTERVAL` (currently 1500ms)
   - Set to 3000ms if you don't need real-time updates
   - Robot battery will last longer

2. **Reduce WiFi Usage**
   - Only connect room ESP32 for alerts
   - Robot uses only ESP-NOW (no WiFi needed)
   - Saved ~100mA of power

3. **Improve Sensor Accuracy**
   - Average multiple readings instead of single
   - Wait for DHT11 to stabilize (slower sensor)
   - Add thermal insulaition around DHT11

## Security Recommendations

1. **WiFi Security**
   - Use WPA2 password (not WEP)
   - Change default WiFi password
   - Keep WiFi network name hidden (SSID broadcast off)

2. **Telegram Security**
   - Don't share bot token or chat ID
   - Regularly refresh bot token via @BotFather
   - Use separate bot account (not personal)

3. **Physical Security**
   - Keep ESP32s in enclosures
   - Don't expose GPIO pins
   - Secure power connections
## Support & Debugging

For issues, check:
1. **Serial Monitor Output** - Most issues shown there
2. **Pin Connections** - Verify all connections match code
3. **Power Supply** - Ensure sufficient current (motors need 500mA+)
4. **Code Comments** - Read detailed comments in .ino files
5. **PROJECT_DESIGN.md** - System architecture reference

## Additional Resources

- ESP32 Documentation: https://docs.espressif.com/projects/esp-idf/en/latest/
- Arduino IDE: https://www.arduino.cc/en/software
- Telegram Bot API: https://core.telegram.org/bots
- HC-SR04 Datasheet: Search online for "HC-SR04 ultrasonic datasheet"
- DHT11 Datasheet: Search online for "DHT11 temperature humidity"



## 🤖 Robot ESP32 - Pin Configuration

| Device | GPIO | Type | Purpose |
|--------|------|------|---------|
| Ultrasonic TRIG | 5 | OUT | Trigger pulse for distance |
| Ultrasonic ECHO | 18 | IN | Echo pulse from distance sensor |
| Motion Sensor | 19 | IN | PIR motion detection |
| Gas Sensor | 34 | ADC | Air quality analog input |
| DHT11 Data | 4 | DATA | Temperature/humidity 1-wire |
| Motor Left 1 | 13 | OUT | Left motor forward |
| Motor Left 2 | 12 | OUT | Left motor backward |
| Motor Right 1 | 14 | OUT | Right motor forward |
| Motor Right 2 | 27 | OUT | Right motor backward |

## 🏠 Room ESP32 - Pin Configuration

| Device | GPIO | Type | Purpose |
|--------|------|------|---------|
| Lights | 26 | OUT | LED relay (motion-triggered) |
| Fan | 25 | OUT | Fan relay (temp-based) |
| AC | 33 | OUT | AC relay (temp-based) |
| Window | 14 | PWM | Servo for window (0°/90°) |
| Local PIR | 27 | IN | Room motion sensor (optional) |

## ⚙️ Key Constants

### Robot Code
```cpp
OBSTACLE_DISTANCE_THRESHOLD = 20 cm
STOP_DELAY_MS = 200 ms
TURN_DELAY_MS = 400 ms
SENSOR_READ_INTERVAL = 1500 ms
ESP_NOW_CHANNEL = 0
```

### Room Code
```cpp
TEMP_FAN_THRESHOLD = 30°C
TEMP_AC_THRESHOLD = 35°C
TEMP_WINDOW_OPEN_THRESHOLD = 30°C
WINDOW_OPEN_ANGLE = 90°
WINDOW_CLOSED_ANGLE = 0°
GAS_ALERT_LEVEL = 2000 ADC
TELEGRAM_ALERT_DELAY = 10000 ms
AUTOMATION_CHECK_INTERVAL = 2000 ms
```

## 📊 Data Structure (11 bytes)

```
Sent every 1.5 seconds from robot to room:
┌─────────────┬──────────┬──────────┬────────┬──────────┐
│Temperature │ Humidity │ Gas Lvl  │ Motion │ Distance │
│  float (4) │ float(4) │  int(2)  │ int(1) │  int(2)  │
└─────────────┴──────────┴──────────┴────────┴──────────┘
```

## 🔄 Operation Flow

### Robot (Every 1.5 seconds)
1. Read all sensors → DHT11, ultrasonic, gas, motion
2. Check distance → if < 20cm, obstacle detected
3. Control motors → forward OR turn right
4. Send data via ESP-NOW → to room controller
5. Wait 1.5s → repeat

### Room (Every 2.0 seconds)
1. Receive ESP-NOW data → from robot
2. Check rules:
   - Lights ON if motion detected
   - Fan ON if temp > 30°C
   - AC ON if temp > 35°C
   - Window OPEN if temp < 30°C
3. Safety check → Gas alert if level > 2000
4. Send Telegram → only if alert needed
5. Wait 2.0s → repeat

## ✅ Automation Rules

| Sensor | Threshold | Action |
|--------|-----------|--------|
| Motion | Detected | Lights ON |
| Temp | > 30°C | Fan ON |
| Temp | > 35°C | AC ON |
| Temp | < 30°C | Window OPEN |
| Gas | > 2000 ADC | Telegram Alert |

## 🛠️ Setup Checklist

- [ ] Install Arduino IDE
- [ ] Add ESP32 board package
- [ ] Find robot ESP32 MAC address
- [ ] Update room.ino with robot MAC (line 47-48)
- [ ] Update room.ino WiFi SSID (line 22)
- [ ] Update room.ino WiFi password (line 23)
- [ ] Create Telegram bot via @BotFather
- [ ] Get your Telegram chat ID
- [ ] Update room.ino bot token (line 24)
- [ ] Update room.ino chat ID (line 25)
- [ ] Upload robo.ino to robot ESP32
- [ ] Upload room.ino to room ESP32
- [ ] Open Serial Monitor (115200 baud)
- [ ] Verify startup messages appear
- [ ] Test lights → move in front of robot
- [ ] Test temperature → heat robot sensor
- [ ] Verify Telegram startup message received

## 🐛 Troubleshooting Quick

| Issue | Check | Solution |
|-------|-------|----------|
| No data in room | Robot sends data? | Check robot serial output |
| | MAC address correct? | Get robot MAC, update room.ino |
| | ESP-NOW initialized? | Look for "✓ ESP-NOW Ready" |
| Lights don't work | Motion sensor working? | Test PIR sensor |
| | Data reaching room? | Verify in room console |
| | GPIO connected? | Check GPIO26 to light relay |
| Temperature not working | DHT11 working? | Check if temp shows (not NaN) |
| | Thresholds reached? | Manually heat sensor |
| | Relay connected? | Check GPIO25/33 for relay |
| WiFi won't connect | SSID correct? | Check WiFi name spelling |
| | Password correct? | Verify WiFi password |
| | 2.4GHz network? | ESP32 needs 2.4GHz only |
| Telegram alerts not working | WiFi connected? | Look for IP address in console |
| | Bot token correct? | Verify in room.ino line 24 |
| | Chat ID correct? | Verify in room.ino line 25 |
| | Gas level high? | Must exceed 2000 ADC value |

## 📱 Serial Monitor Baud Rates

- Robot: **115200**
- Room: **115200**
- Both: **115200** ← All use same rate

## 🔧 Important Variables

### Robot
```cpp
sensor_readings.temperature     // °C
sensor_readings.humidity        // %
sensor_readings.gas_level       // ADC (0-4095)
sensor_readings.motion_detected // 0 or 1
sensor_readings.distance_to_obstacle // cm
```

### Room
```cpp
incoming_sensor_data.*          // Same fields as above
last_gas_alert_time             // Timestamp to throttle alerts
```

## 📡 ESP-NOW Details

- **Protocol**: Low-latency wireless mesh
- **Channel**: 0 (WiFi channel)
- **Range**: ~250m line of sight
- **Bandwidth**: ~11 bytes every 1.5s ≈ 73 bps
- **Latency**: 5-50ms typical
- **Encryption**: Disabled (local network)
- **MAC Address**: Both devices need each other's MAC in code

## 🔋 Power Consumption

| Device | Idle | Active | Duration |
|--------|------|--------|----------|
| Robot | 150mA | 350mA | 2-3 hrs |
| Room | 200mA | 500mA | Continuous |