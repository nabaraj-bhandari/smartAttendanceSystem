#include <SPI.h>
#include <MFRC522.h>
#include "WiFi.h"
#include <HTTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Pin Definitions
#define SS_PIN 5
#define RST_PIN 4
#define BUZZER_PIN 27
#define GREEN_LED_PIN 26
#define RED_LED_PIN 25

// Wi-Fi Credentials
const char *ssid = "Bed_DHFibernet";
const char *password = "AR27@69MY";

// Backend API URL
String api_url = "https://smarthajiri-api.onrender.com/api/logs";

// RFID UID Storage
char str[32] = "";
String UID_Result = "--------";

// Create MFRC522 and LCD Objects
MFRC522 mfrc522(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Function Prototypes
void byteArray_to_string(byte array[], unsigned int len, char buffer[]);
void connectToWiFi();
void http_Req(String str_uid);
int getUID();

void setup()
{
    // Initialize I2C with ESP32's default pins: SDA = 21, SCL = 22
    Wire.begin(21, 22);
    lcd.init();
    lcd.backlight();
    lcd.print("Starting...");

    // Initialize LEDs and Buzzer
    pinMode(RED_LED_PIN, OUTPUT);
    pinMode(GREEN_LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);

    // Connect to Wi-Fi
    connectToWiFi();

    // Initialize RFID Reader
    SPI.begin();
    mfrc522.PCD_Init();

    lcd.clear();
    lcd.print("Scan a card...");
}

void loop()
{
    if (getUID())
    {
        lcd.clear();
        lcd.print("UID:");
        lcd.setCursor(0, 1);
        lcd.print(UID_Result);

        // Send UID to backend API
        http_Req(UID_Result);

        delay(200); // Prevent rapid scanning

        lcd.clear();
        lcd.print("Scan a card...");
    }
}

// Connect to Wi-Fi Network
void connectToWiFi()
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    lcd.clear();
    lcd.print("Connecting...");

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
    }

    lcd.clear();
    lcd.print("WiFi Connected");
    delay(100);
}

// Get RFID UID
int getUID()
{
    if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial())
        return 0;

    byteArray_to_string(mfrc522.uid.uidByte, mfrc522.uid.size, str);
    UID_Result = str;

    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();

    return 1;
}

// Convert Byte Array to String
void byteArray_to_string(byte array[], unsigned int len, char buffer[])
{
    for (unsigned int i = 0; i < len; i++)
    {
        buffer[i * 2] = (array[i] >> 4) < 0xA ? '0' + (array[i] >> 4) : 'A' + (array[i] >> 4) - 0xA;
        buffer[i * 2 + 1] = (array[i] & 0x0F) < 0xA ? '0' + (array[i] & 0x0F) : 'A' + (array[i] & 0x0F) - 0xA;
    }
    buffer[len * 2] = '\0';
}

// Send HTTP Request to Backend API
void http_Req(String str_uid)
{
    if (WiFi.status() != WL_CONNECTED)
    {
        lcd.clear();
        lcd.print("WiFi Disconnected");
        delay(3000);
        return;
    }

    String payload = "{\"uid\": \"" + str_uid + "\"}";

    HTTPClient http;
    http.begin(api_url);
    http.addHeader("Content-Type", "application/json");

    int httpCode = http.POST(payload);

    lcd.clear();
    if (httpCode > 0)
    {
        lcd.print("Sent UID");
        lcd.setCursor(0, 1);
        lcd.print("Success!");

        digitalWrite(GREEN_LED_PIN, HIGH);
        for (int i = 0; i < 2; i++)
        {
            digitalWrite(BUZZER_PIN, HIGH);
            delay(200);
            digitalWrite(BUZZER_PIN, LOW);
            delay(200);
        }
        digitalWrite(GREEN_LED_PIN, LOW);
    }
    else
    {
        lcd.print("Send Failed");
        digitalWrite(RED_LED_PIN, HIGH);

        for (int i = 0; i < 3; i++)
        {
            digitalWrite(BUZZER_PIN, HIGH);
            delay(200);
            digitalWrite(BUZZER_PIN, LOW);
            delay(200);
        }
        digitalWrite(RED_LED_PIN, LOW);
    }
}
