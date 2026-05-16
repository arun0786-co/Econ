/************************************************************
 * AUTONOMOUS ROBOT ESP32 - ENVIRONMENTAL MONITOR
 *
 * Purpose: Collects environmental data and navigates autonomously
 * Communication: ESP-NOW protocol to Room ESP32
 * Sensors: Temperature, Humidity, Gas, Motion, Distance
 * Actuators: DC Motors (L298N driver)
 *
 * Author Notes:
 *   - This robot moves throughout the environment
 *   - Sends real-time sensor data every 1.5 seconds
 *   - Avoids obstacles automatically
 *   - Reports to central room controller
 ************************************************************/

#include <WiFi.h>
#include <esp_now.h>
#include <DHT.h>

/* ============================================
 * HARDWARE PIN CONFIGURATION
 * ============================================ */

// Ultrasonic Sensor Pins (HC-SR04)
#define ULTRASONIC_TRIG_PIN 5  // Trigger pulse output
#define ULTRASONIC_ECHO_PIN 18 // Echo pulse input

// Motion Sensor Pin (HC-SR501 PIR)
#define MOTION_SENSOR_PIN 19

// Gas Sensor Pin (MQ-X Series - Analog)
#define GAS_SENSOR_PIN 34 // ADC pin for analog reading

// Temperature & Humidity Sensor (DHT11)
#define DHT_DATA_PIN 4
#define DHT_SENSOR_TYPE DHT11

// Motor Driver Pins (L298N Module)
// Motor A (Left side)
#define MOTOR_LEFT_PIN1 13
#define MOTOR_LEFT_PIN2 12
// Motor B (Right side)
#define MOTOR_RIGHT_PIN1 14
#define MOTOR_RIGHT_PIN2 27

/* ============================================
 * SYSTEM CONFIGURATION & THRESHOLDS
 * ============================================ */

// Obstacle Detection Parameters
#define OBSTACLE_DISTANCE_THRESHOLD 20 // cm - trigger avoidance
#define OBSTACLE_CHECK_TIMEOUT 30000   // microseconds
#define TURN_DELAY_MS 400              // Time to turn away from obstacle
#define STOP_DELAY_MS 200              // Time to pause before turning

// Data Transmission Parameters
#define SENSOR_READ_INTERVAL 1500 // milliseconds between readings
#define ESP_NOW_CHANNEL 0         // WiFi channel (must match room esp32)

// Sensor Calibration
#define GAS_SENSOR_MAX_READING 4095      // Maximum ADC value
#define DISTANCE_CONVERSION_FACTOR 0.034 // Speed of sound calculation

// Motion & Status Indicators
#define MOVEMENT_FORWARD 1
#define MOVEMENT_STOPPED 0
#define MOVEMENT_TURNING 2

/* ============================================
 * DATA STRUCTURES
 * ============================================ */

// Sensor readings packet sent to room controller
typedef struct robot_sensor_data
{
    float temperature;        // °C from DHT11
    float humidity;           // % RH from DHT11
    int gas_level;            // 0-4095 ADC reading from MQ sensor
    int motion_detected;      // 1 if motion, 0 if none
    int distance_to_obstacle; // cm from ultrasonic sensor
} robot_sensor_data;

// Global instance to store current sensor data
robot_sensor_data sensor_readings;

// Room ESP32 MAC address - used for ESP-NOW addressing
// YOU MUST UPDATE THIS WITH YOUR ROOM ESP32's MAC ADDRESS
uint8_t room_esp32_mac[] = {0x24, 0x6F, 0x28, 0xAA, 0xBB, 0xCC};

/* ============================================
 * SENSOR OBJECTS & INSTANCES
 * ============================================ */

// DHT11 sensor instance for temperature/humidity
DHT dht_sensor(DHT_DATA_PIN, DHT_SENSOR_TYPE);

/* ============================================
 * ESP-NOW INITIALIZATION & SETUP
 *
 * We're using ESP-NOW for low-latency, energy-efficient
 * communication with the room controller. No WiFi needed!
 * ============================================ */

void setup_espnow_communication()
{
    // Set device to WiFi Station mode (not access point)
    WiFi.mode(WIFI_STA);

    // Initialize ESP-NOW protocol
    if (esp_now_init() != ESP_OK)
    {
        Serial.println("ERROR: ESP-NOW initialization failed!");
        return;
    }

    // Prepare peer information for room controller
    esp_now_peer_info_t peer_info = {};
    memcpy(peer_info.peer_addr, room_esp32_mac, 6);
    peer_info.channel = ESP_NOW_CHANNEL;
    peer_info.encrypt = false; // Local network, encryption not needed

    // Register the room controller as a peer
    if (esp_now_add_peer(&peer_info) != ESP_OK)
    {
        Serial.println("ERROR: Failed to register room controller as peer!");
        return;
    }

    Serial.println("✓ ESP-NOW Ready - Robot is broadcasting sensor data");
}

/* ============================================
 * MOTOR CONTROL FUNCTIONS
 *
 * These functions control the L298N motor driver
 * to move the robot in different directions.
 *
 * Motor Logic:
 *   IN1=HIGH, IN2=LOW  → Motor spins forward
 *   IN1=LOW, IN2=HIGH  → Motor spins backward
 *   IN1=HIGH, IN2=HIGH → Motor stops
 *   IN1=LOW, IN2=LOW   → Motor stops
 * ============================================ */

// Move the robot forward (both motors forward)
void move_forward()
{
    digitalWrite(MOTOR_LEFT_PIN1, HIGH);
    digitalWrite(MOTOR_LEFT_PIN2, LOW);
    digitalWrite(MOTOR_RIGHT_PIN1, HIGH);
    digitalWrite(MOTOR_RIGHT_PIN2, LOW);
}

// Turn the robot right (left motor forward, right motor backward)
void turn_right()
{
    digitalWrite(MOTOR_LEFT_PIN1, HIGH);
    digitalWrite(MOTOR_LEFT_PIN2, LOW);
    digitalWrite(MOTOR_RIGHT_PIN1, LOW);
    digitalWrite(MOTOR_RIGHT_PIN2, HIGH);
}

// Stop all motors immediately
void stop_all_motors()
{
    digitalWrite(MOTOR_LEFT_PIN1, LOW);
    digitalWrite(MOTOR_LEFT_PIN2, LOW);
    digitalWrite(MOTOR_RIGHT_PIN1, LOW);
    digitalWrite(MOTOR_RIGHT_PIN2, LOW);
}

/* ============================================
 * ULTRASONIC DISTANCE MEASUREMENT
 *
 * This function measures distance using the HC-SR04 sensor.
 *
 * How it works:
 *   1. Send 10μs pulse on TRIG pin
 *   2. ECHO pin goes HIGH when pulse reflects back
 *   3. Measure time HIGH = duration
 *   4. Calculate distance: duration * 0.034 / 2
 *
 * Returns: Distance in centimeters
 *          999 if timeout (no obstacle detected)
 * ============================================ */

int read_distance_cm()
{
    // Send trigger pulse to sensor
    digitalWrite(ULTRASONIC_TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(ULTRASONIC_TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(ULTRASONIC_TRIG_PIN, LOW);

    // Wait for echo pulse and measure its duration
    long echo_duration = pulseIn(ULTRASONIC_ECHO_PIN, HIGH, OBSTACLE_CHECK_TIMEOUT);

    // Check if we got a valid reading
    if (echo_duration == 0)
    {
        return 999; // No object detected in range
    }

    // Convert time to distance in centimeters
    // Formula: distance = (time * speed of sound) / 2
    // Speed of sound ≈ 0.034 cm/μs, divide by 2 for round trip
    int distance_cm = echo_duration * DISTANCE_CONVERSION_FACTOR / 2;

    return distance_cm;
}

/* ============================================
 * ARDUINO SETUP - Hardware & Peripheral Initialization
 *
 * This runs once when the board powers on.
 * We initialize all sensors, actuators, and communication.
 * ============================================ */

void setup()
{
    // Initialize serial communication for debugging
    Serial.begin(115200);
    delay(100);

    Serial.println("\n\n================================");
    Serial.println("🤖 ROBOT ESP32 STARTING UP");
    Serial.println("================================");

    /* Initialize Sensor Input Pins */
    pinMode(ULTRASONIC_TRIG_PIN, OUTPUT);
    pinMode(ULTRASONIC_ECHO_PIN, INPUT);
    pinMode(MOTION_SENSOR_PIN, INPUT);
    pinMode(GAS_SENSOR_PIN, INPUT); // ADC - no need to set pinMode

    /* Initialize Motor Driver Output Pins */
    pinMode(MOTOR_LEFT_PIN1, OUTPUT);
    pinMode(MOTOR_LEFT_PIN2, OUTPUT);
    pinMode(MOTOR_RIGHT_PIN1, OUTPUT);
    pinMode(MOTOR_RIGHT_PIN2, OUTPUT);

    // Ensure motors are stopped at startup
    stop_all_motors();

    /* Initialize DHT11 Temperature/Humidity Sensor */
    dht_sensor.begin();
    delay(500); // Let sensor stabilize

    /* Initialize ESP-NOW Communication */
    setup_espnow_communication();

    Serial.println("✓ All systems initialized successfully");
    Serial.println("✓ Robot ready for autonomous operation\n");
}

/* ============================================
 * MAIN PROGRAM LOOP - Continuous Operation
 *
 * This function runs repeatedly while the robot is powered.
 * Each cycle:
 *   1. Read all available sensors (temp, humidity, gas, motion, distance)
 *   2. Make autonomous navigation decisions based on distance
 *   3. Send collected data to room controller via ESP-NOW
 *   4. Wait before next cycle (1.5 seconds)
 * ============================================ */

void loop()
{
    // ===== SENSOR DATA COLLECTION =====
    // Read temperature and humidity from DHT11
    sensor_readings.temperature = dht_sensor.readTemperature();
    sensor_readings.humidity = dht_sensor.readHumidity();

    // Read raw gas sensor analog value (0-4095)
    sensor_readings.gas_level = analogRead(GAS_SENSOR_PIN);

    // Read motion detection (HIGH if motion, LOW if no motion)
    sensor_readings.motion_detected = digitalRead(MOTION_SENSOR_PIN);

    // Measure distance to nearest obstacle
    sensor_readings.distance_to_obstacle = read_distance_cm();

    // ===== AUTONOMOUS NAVIGATION LOGIC =====
    // Simple obstacle avoidance algorithm:
    // If something is close, stop and turn right
    // Otherwise, keep moving forward

    if (sensor_readings.distance_to_obstacle < OBSTACLE_DISTANCE_THRESHOLD)
    {
        // Obstacle detected! Begin avoidance maneuver
        stop_all_motors();
        delay(STOP_DELAY_MS); // Pause briefly
        turn_right();         // Turn away from obstacle
        delay(TURN_DELAY_MS); // Execute turn
    }
    else
    {
        // Path is clear, move forward
        move_forward();
    }

    // ===== SEND DATA TO ROOM CONTROLLER =====
    // Transmit sensor readings via ESP-NOW
    // The room ESP32 will receive this and make decisions
    esp_now_send(room_esp32_mac, (uint8_t *)&sensor_readings, sizeof(sensor_readings));

    // Optional: Debug output to serial monitor
    Serial.print("📡 Sent - Temp: ");
    Serial.print(sensor_readings.temperature);
    Serial.print("°C | Dist: ");
    Serial.print(sensor_readings.distance_to_obstacle);
    Serial.println(" cm");

    // Wait before next sensor reading cycle
    delay(SENSOR_READ_INTERVAL);
}