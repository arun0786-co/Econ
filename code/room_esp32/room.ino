/************************************************************
 * SMART ROOM CONTROLLER ESP32 - Home Automation Hub
 *
 * Purpose: Receives sensor data from robot, controls home devices,
 *          monitors safety, and sends Telegram alerts
 * Communication: ESP-NOW (receiver) + WiFi (for Telegram alerts)
 * Actuators: LED lights, Fan, AC, Servo (window)
 * Features: Motion detection, temperature control, gas safety
 *
 * Author Notes:
 *   - This is the "brain" of the smart home system
 *   - Runs automation rules based on sensor data from robot
 *   - Handles emergency alerts and notifications
 *   - Always connected to room's WiFi for cloud communication
 ************************************************************/

#include <WiFi.h>
#include <esp_now.h>
#include <HTTPClient.h>
#include <Servo.h>

/* ============================================
 * WIFI & CLOUD SERVICES CONFIGURATION
 *
 * Configure these with your actual credentials
 * before uploading to your ESP32!
 * ============================================ */

// WiFi Network Credentials
const char *wifi_ssid = "YOUR_WIFI_NAME";
const char *wifi_password = "YOUR_WIFI_PASSWORD";

// Telegram Bot Configuration
String telegram_bot_token = "YOUR_BOT_TOKEN";
String telegram_chat_id = "YOUR_CHAT_ID";

/* ============================================
 * HARDWARE PIN CONFIGURATION
 * ============================================ */

// Smart Home Actuator Pins
#define LIGHT_PIN 26     // LED lights - GPIO26
#define FAN_PIN 25       // Cooling fan motor - GPIO25
#define AC_PIN 33        // Air conditioner relay - GPIO33
#define SERVO_PIN 14     // Window servo motor - GPIO14
#define LOCAL_PIR_PIN 27 // Room's motion sensor - GPIO27

/* ============================================
 * AUTOMATION THRESHOLDS & CONSTANTS
 *
 * These values control when various systems activate.
 * Adjust these based on your climate and preferences.
 * ============================================ */

// Temperature Control Thresholds (in Celsius)
#define TEMP_FAN_THRESHOLD 30         // Turn on fan above this
#define TEMP_AC_THRESHOLD 35          // Turn on AC above this
#define TEMP_WINDOW_OPEN_THRESHOLD 30 // Close window above this

// Window Servo Positions (0-180 degrees)
#define WINDOW_CLOSED_ANGLE 0 // Window fully closed
#define WINDOW_OPEN_ANGLE 90  // Window fully open

// Safety & Alert Thresholds
#define GAS_ALERT_LEVEL 2000       // ADC value for gas alert
#define TELEGRAM_ALERT_DELAY 10000 // milliseconds between alerts (prevent spam)

// Communication Timing
#define AUTOMATION_CHECK_INTERVAL 2000 // milliseconds between checks

/* ============================================
 * DATA STRUCTURES
 * ============================================ */

// Sensor data received from robot (matches robot's struct)
typedef struct received_robot_data
{
    float temperature;        // °C from robot's DHT11
    float humidity;           // % RH from robot's DHT11
    int gas_level;            // 0-4095 ADC reading
    int motion_detected;      // 1 if motion detected, 0 if none
    int distance_to_obstacle; // cm measurement
} received_robot_data;

// Global buffer for incoming sensor data
received_robot_data incoming_sensor_data;

// Timestamp tracking for alert throttling
unsigned long last_gas_alert_time = 0;

/* ============================================
 * HARDWARE OBJECTS & INSTANCES
 * ============================================ */

// Servo motor object for window control
Servo window_servo;

/* ============================================
 * ESP-NOW RECEIVE CALLBACK
 *
 * This function is called automatically whenever
 * the robot sends new sensor data via ESP-NOW.
 * We just copy the data to our buffer for processing.
 * ============================================ */

void on_data_received(const uint8_t *mac_address, const uint8_t *incoming_data_buffer, int data_length)
{
    // Copy incoming data to our global buffer for the main loop to process
    memcpy(&incoming_sensor_data, incoming_data_buffer, sizeof(incoming_sensor_data));

    // Debug: Show that we received data
    Serial.print("✓ Received data from Robot - Temp: ");
    Serial.print(incoming_sensor_data.temperature);
    Serial.println("°C");
}

/* ============================================
 * ESP-NOW INITIALIZATION
 *
 * Sets up this ESP32 to receive data from the robot.
 * No peer configuration needed - we accept from any sender!
 * ============================================ */

void setup_espnow_receiver()
{
    // Set device to WiFi Station mode
    WiFi.mode(WIFI_STA);

    // Initialize ESP-NOW protocol
    if (esp_now_init() != ESP_OK)
    {
        Serial.println("ERROR: ESP-NOW initialization failed!");
        return;
    }

    // Register the callback function for receiving data
    esp_now_register_recv_cb(on_data_received);

    Serial.println("✓ ESP-NOW Ready - Waiting for robot data...");
}

/* ============================================
 * TELEGRAM ALERT SYSTEM
 *
 * Sends alert messages to your phone via Telegram Bot API.
 * Only sends if WiFi is connected.
 *
 * Setup:
 *   1. Create a bot with @BotFather on Telegram
 *   2. Get your Chat ID from @userinfobot
 *   3. Update telegram_bot_token and telegram_chat_id above
 * ============================================ */

void send_telegram_alert(String alert_message)
{
    // Only send if WiFi is connected
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("⚠️  WiFi not connected - cannot send alert");
        return;
    }

    // Build Telegram API URL
    HTTPClient http;
    String api_url = "https://api.telegram.org/bot" + telegram_bot_token +
                     "/sendMessage?chat_id=" + telegram_chat_id +
                     "&text=" + alert_message;

    // Send HTTP GET request to Telegram
    http.begin(api_url);
    int response_code = http.GET();

    if (response_code == 200)
    {
        Serial.println("✓ Alert sent to Telegram");
    }
    else
    {
        Serial.print("⚠️  Telegram error - Code: ");
        Serial.println(response_code);
    }

    http.end();
}

/* ============================================
 * ARDUINO SETUP - Hardware & Peripheral Initialization
 *
 * This runs once when the board powers on.
 * We initialize all actuators, WiFi, and communication.
 * ============================================ */

void setup()
{
    // Initialize serial communication for debugging
    Serial.begin(115200);
    delay(100);

    Serial.println("\n\n================================");
    Serial.println("🏠 SMART ROOM CONTROLLER STARTING");
    Serial.println("================================");

    /* Initialize Actuator Output Pins */
    pinMode(LIGHT_PIN, OUTPUT);
    pinMode(FAN_PIN, OUTPUT);
    pinMode(AC_PIN, OUTPUT);
    pinMode(LOCAL_PIR_PIN, INPUT);

    // Ensure all devices are OFF at startup
    digitalWrite(LIGHT_PIN, LOW);
    digitalWrite(FAN_PIN, LOW);
    digitalWrite(AC_PIN, LOW);

    /* Initialize Window Servo Motor */
    window_servo.attach(SERVO_PIN);
    window_servo.write(WINDOW_CLOSED_ANGLE); // Start with window closed
    delay(500);                              // Let servo move to position

    /* Connect to WiFi Network */
    Serial.print("Connecting to WiFi: ");
    Serial.println(wifi_ssid);
    WiFi.begin(wifi_ssid, wifi_password);

    // Wait for WiFi connection (with timeout)
    int wifi_attempts = 0;
    while (WiFi.status() != WL_CONNECTED && wifi_attempts < 20)
    {
        delay(500);
        Serial.print(".");
        wifi_attempts++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("\n✓ WiFi connected!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
    }
    else
    {
        Serial.println("\n⚠️  WiFi connection failed - alerts will not work");
    }

    /* Initialize ESP-NOW for Receiving Robot Data */
    setup_espnow_receiver();

    // Send startup notification
    delay(1000);
    send_telegram_alert("✅ Smart Room System Started");

    Serial.println("✓ All systems initialized successfully");
    Serial.println("✓ Room controller ready for automation\n");
}

/* ============================================
 * MAIN PROGRAM LOOP - Continuous Automation
 *
 * This function runs repeatedly and implements
 * the home automation rules based on sensor data
 * received from the robot.
 *
 * Automation Flow:
 *   1. Lighting: Controlled by robot's motion detection
 *   2. Fan: Turns on when temperature > 30°C
 *   3. AC: Turns on when temperature > 35°C
 *   4. Window: Opens/closes based on temperature
 *   5. Safety: Monitors gas levels and alerts
 * ============================================ */

void loop()
{
    // ===== LIGHTING CONTROL =====
    // Turn lights on when motion is detected, off otherwise
    if (incoming_sensor_data.motion_detected == HIGH)
    {
        digitalWrite(LIGHT_PIN, HIGH); // Motion detected - lights ON
    }
    else
    {
        digitalWrite(LIGHT_PIN, LOW); // No motion - lights OFF
    }

    // ===== TEMPERATURE-BASED CLIMATE CONTROL =====
    // Fan activation logic
    if (incoming_sensor_data.temperature > TEMP_FAN_THRESHOLD)
    {
        digitalWrite(FAN_PIN, HIGH); // Temp too high - fan ON
    }
    else
    {
        digitalWrite(FAN_PIN, LOW); // Temp acceptable - fan OFF
    }

    // AC activation logic
    if (incoming_sensor_data.temperature > TEMP_AC_THRESHOLD)
    {
        digitalWrite(AC_PIN, HIGH); // Temp critical - AC ON
    }
    else
    {
        digitalWrite(AC_PIN, LOW); // Temp normal - AC OFF
    }

    // ===== WINDOW AUTOMATION =====
    // Window servo control based on temperature
    if (incoming_sensor_data.temperature < TEMP_WINDOW_OPEN_THRESHOLD)
    {
        window_servo.write(WINDOW_OPEN_ANGLE); // Temp low - open window
    }
    else
    {
        window_servo.write(WINDOW_CLOSED_ANGLE); // Temp high - close window
    }

    // ===== GAS/SMOKE SAFETY MONITORING =====
    // Alert if dangerous gas levels detected
    if (incoming_sensor_data.gas_level > GAS_ALERT_LEVEL)
    {
        // Check if we've already sent an alert recently
        if ((millis() - last_gas_alert_time) > TELEGRAM_ALERT_DELAY)
        {
            send_telegram_alert("⚠️ ALERT: High gas/smoke level detected!");
            last_gas_alert_time = millis(); // Update last alert time
        }
    }

    // Wait before next automation check
    delay(AUTOMATION_CHECK_INTERVAL);
}