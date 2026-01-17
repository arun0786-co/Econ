/************************************************************
 * FILE: room_controller.ino
 * ROLE: Smart Room ESP32 (Decision + Alert Node)
 * COMMUNICATION: ESP-NOW (Receiver)
 ************************************************************/

#include <WiFi.h>
#include <esp_now.h>
#include <HTTPClient.h>
#include <Servo.h>

/* ---------------- WIFI & TELEGRAM ---------------- */

const char *ssid = "YOUR_WIFI_NAME";
const char *password = "YOUR_WIFI_PASSWORD";

String botToken = "YOUR_BOT_TOKEN";
String chatID = "YOUR_CHAT_ID";

/* ---------------- PIN DEFINITIONS ---------------- */

#define LED_PIN 26
#define FAN_PIN 25
#define AC_PIN 33
#define SERVO_PIN 14
#define PIR_PIN 27

/* ---------------- DATA STRUCTURE ---------------- */

typedef struct struct_message
{
    float temperature;
    float humidity;
    int gasLevel;
    int motion;
    int distance;
} struct_message;

struct_message incomingData;

/* ---------------- OBJECTS ---------------- */
Servo windowServo;

/* ---------------- ESP-NOW CALLBACK ---------------- */

void onReceive(const uint8_t *mac, const uint8_t *incomingDataRaw, int len)
{
    memcpy(&incomingData, incomingDataRaw, sizeof(incomingData));
}

/* ---------------- ESP-NOW SETUP ---------------- */

void setupESPNow()
{
    WiFi.mode(WIFI_STA);

    if (esp_now_init() != ESP_OK)
    {
        Serial.println("ESP-NOW Initialization Failed");
        return;
    }

    esp_now_register_recv_cb(onReceive);
    Serial.println("ESP-NOW Ready (Room)");
}

/* ---------------- TELEGRAM FUNCTION ---------------- */

void sendTelegram(String message)
{
    if (WiFi.status() == WL_CONNECTED)
    {
        HTTPClient http;
        String url = "https://api.telegram.org/bot" + botToken +
                     "/sendMessage?chat_id=" + chatID +
                     "&text=" + message;
        http.begin(url);
        http.GET();
        http.end();
    }
}

/* ---------------- SETUP ---------------- */

void setup()
{
    Serial.begin(115200);

    pinMode(LED_PIN, OUTPUT);
    pinMode(FAN_PIN, OUTPUT);
    pinMode(AC_PIN, OUTPUT);
    pinMode(PIR_PIN, INPUT);

    windowServo.attach(SERVO_PIN);
    windowServo.write(0); // window closed

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    setupESPNow();
    sendTelegram("✅ Smart Room System Started");
}

/* ---------------- LOOP ---------------- */

void loop()
{
    // Lighting control
    digitalWrite(LED_PIN, incomingData.motion ? HIGH : LOW);

    // Fan control
    digitalWrite(FAN_PIN, incomingData.temperature > 30 ? HIGH : LOW);

    // AC control
    digitalWrite(AC_PIN, incomingData.temperature > 35 ? HIGH : LOW);

    // Window automation
    if (incomingData.temperature < 30)
    {
        windowServo.write(90); // Open
    }
    else
    {
        windowServo.write(0); // Close
    }

    // Gas alert
    if (incomingData.gasLevel > 2000)
    {
        sendTelegram("⚠️ ALERT: Smoke / CO detected!");
        delay(10000); // prevent spam
    }

    delay(2000);
}