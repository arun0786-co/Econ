/************************************************************
 * FILE: robot_controller.ino
 * ROLE: Autonomous Robot ESP32 (Sensor + Navigation Node)
 * COMMUNICATION: ESP-NOW (Sender)
 ************************************************************/

#include <WiFi.h>
#include <esp_now.h>
#include <DHT.h>

/* ---------------- PIN DEFINITIONS ---------------- */

// Ultrasonic Sensor
#define TRIG_PIN 5
#define ECHO_PIN 18

// PIR Sensor
#define PIR_PIN 19

// Gas Sensor (MQ series)
#define GAS_PIN 34 // ADC pin

// DHT11
#define DHTPIN 4
#define DHTTYPE DHT11

// L298N Motor Driver
#define IN1 13
#define IN2 12
#define IN3 14
#define IN4 27

/* ---------------- DATA STRUCTURE ---------------- */

typedef struct struct_message
{
    float temperature;
    float humidity;
    int gasLevel;
    int motion;
    int distance;
} struct_message;

struct_message robotData;

/* ---------------- ROOM ESP32 MAC ADDRESS ---------------- */
/* REPLACE THIS WITH YOUR ROOM ESP32 MAC ADDRESS */
uint8_t roomAddress[] = {0x24, 0x6F, 0x28, 0xAA, 0xBB, 0xCC};

/* ---------------- OBJECTS ---------------- */
DHT dht(DHTPIN, DHTTYPE);

/* ---------------- ESP-NOW SETUP ---------------- */

void setupESPNow()
{
    WiFi.mode(WIFI_STA);

    if (esp_now_init() != ESP_OK)
    {
        Serial.println("ESP-NOW Initialization Failed");
        return;
    }

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, roomAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK)
    {
        Serial.println("Failed to add peer");
        return;
    }

    Serial.println("ESP-NOW Ready (Robot)");
}

/* ---------------- MOVEMENT FUNCTIONS ---------------- */

void moveForward()
{
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
}

void turnRight()
{
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
}

void stopCar()
{
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
}

/* ---------------- DISTANCE FUNCTION ---------------- */

long getDistance()
{
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    long duration = pulseIn(ECHO_PIN, HIGH, 30000);
    if (duration == 0)
        return 999;
    return duration * 0.034 / 2;
}

/* ---------------- SETUP ---------------- */

void setup()
{
    Serial.begin(115200);

    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    pinMode(PIR_PIN, INPUT);
    pinMode(GAS_PIN, INPUT);

    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    pinMode(IN3, OUTPUT);
    pinMode(IN4, OUTPUT);

    dht.begin();
    setupESPNow();

    Serial.println("Robot ESP32 Started");
}

/* ---------------- LOOP ---------------- */

void loop()
{
    robotData.temperature = dht.readTemperature();
    robotData.humidity = dht.readHumidity();
    robotData.gasLevel = analogRead(GAS_PIN);
    robotData.motion = digitalRead(PIR_PIN);
    robotData.distance = getDistance();

    // Autonomous navigation
    if (robotData.distance < 20)
    {
        stopCar();
        delay(200);
        turnRight();
        delay(400);
    }
    else
    {
        moveForward();
    }

    // Send data to Room ESP32
    esp_now_send(roomAddress, (uint8_t *)&robotData, sizeof(robotData));

    Serial.println("Data sent to Room ESP32");
    delay(1500);
}