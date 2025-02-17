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
#define RED_LED_PIN 25
#define GREEN_LED_PIN 26

// Wi-Fi Credentials
const char *ssid = "Bed_DHFibernet";
const char *password = "AR27@69MY";

// Google Script Web App URL
String Web_App_URL = "https://script.google.com/macros/s/AKfycbx2UIrgwV3ih8jL7ee1e8YBGWIyXPKswORViJwYgmf-2TWvtc_pzn3hro7mWWz68ZlOAQ/exec";

// RFID UID Storage
int readsuccess;
char str[32] = "";
String UID_Result = "--------";

// Mode Selection
String modes = "atc";

// Create MFRC522 and LCD Objects
MFRC522 mfrc522(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Function Prototypes
void byteArray_to_string(byte array[], unsigned int len, char buffer[]);
void connectToWiFi();
void http_Req(String str_modes, String str_uid);
int getUID();

void setup()
{
    // Initialize I2C with ESP32's default pins: SDA = 21, SCL = 22
    Wire.begin(21, 22);
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
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
    lcd.setCursor(0, 0);
    lcd.print("Scan a card...");
    delay(1000);
}

void loop()
{
    if (getUID())
    {
        // Display UID on LCD
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("UID:");
        lcd.setCursor(0, 1);
        lcd.print(UID_Result);

        // Send UID to Google Sheets
        http_Req(modes, UID_Result);

        // Indicate operation status with LED & Buzzer
        if (modes == "atc")
        {
            digitalWrite(GREEN_LED_PIN, HIGH);
            digitalWrite(BUZZER_PIN, HIGH);
            delay(500);
            digitalWrite(BUZZER_PIN, LOW);
            delay(2000);
            digitalWrite(GREEN_LED_PIN, LOW);
        }
        else
        {
            digitalWrite(RED_LED_PIN, HIGH);
            digitalWrite(BUZZER_PIN, HIGH);
            delay(1000);
            digitalWrite(BUZZER_PIN, LOW);
            digitalWrite(RED_LED_PIN, LOW);
        }

        delay(2000); // Delay before next scan
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Scan a card...");
    }
}

// Connect to Wi-Fi Network
void connectToWiFi()
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Connecting...");

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
    }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Connected");
    delay(1000);
}

// Get RFID UID
int getUID()
{
    if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial())
    {
        return 0;
    }

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
        byte nib1 = (array[i] >> 4) & 0x0F;
        byte nib2 = (array[i] >> 0) & 0x0F;
        buffer[i * 2] = nib1 < 0xA ? '0' + nib1 : 'A' + nib1 - 0xA;
        buffer[i * 2 + 1] = nib2 < 0xA ? '0' + nib2 : 'A' + nib2 - 0xA;
    }
    buffer[len * 2] = '\0';
}

// Send HTTP Request to Google Sheets
void http_Req(String str_modes, String str_uid)
{
    if (WiFi.status() != WL_CONNECTED)
    {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("WiFi Disconnected");
        delay(3000);
        return;
    }

    String http_req_url = Web_App_URL + "?sts=" + str_modes + "&uid=" + str_uid;

    HTTPClient http;
    http.begin(http_req_url.c_str());
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    int httpCode = http.GET();

    lcd.clear();
    lcd.setCursor(0, 0);

    if (httpCode > 0)
    {
        lcd.print("Sent UID");
        lcd.setCursor(0, 1);
        lcd.print("Success!");
    }
    else
    {
        lcd.print("Send Failed");
    }

    delay(2000);
}
