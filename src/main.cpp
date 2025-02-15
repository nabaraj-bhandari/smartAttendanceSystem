#include <Arduino.h>
//******************************* Libraries ********************************
// RFID -----------------------------
#include <SPI.h>
#include <MFRC522.h>
// ESP32 ---------------------------
#include <WiFi.h>
#include <HTTPClient.h>
//************************************************************************
#define SS_PIN 5   // GPIO 5 (Slave Select)
#define RST_PIN 15 // GPIO 15 (Reset)
#define LED_PIN 22 // GPIO 22 (D22 on ESP32)
//************************************************************************
MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance
WiFiClient client;                // Create WiFiClient object
//************************************************************************
/* Wi-Fi Credentials */
const char *ssid = "Bed_DHFibernet";
const char *password = "AR27@69MY";
const char *device_token = "83f922e2ef87bc37";
//************************************************************************
String URL = "http://192.168.18.231/rfidattendance/getdata.php";
String getData, Link;
String OldCardID = "";
unsigned long previousMillis = 0;
//************************************************************************
// Function Prototypes
void connectToWiFi();
void SendCardID(String Card_uid);
//************************************************************************
void setup()
{
    delay(1000);
    Serial.begin(115200);     // Start serial communication on COM5
    SPI.begin();              // Init SPI bus
    mfrc522.PCD_Init();       // Init MFRC522 card
    pinMode(LED_PIN, OUTPUT); // Initialize LED pin as output
    connectToWiFi();          // Connect to Wi-Fi
}
//************************************************************************
void loop()
{
    // Check if there's a connection to Wi-Fi or not
    if (WiFi.status() != WL_CONNECTED)
    {
        digitalWrite(LED_PIN, LOW);
        connectToWiFi(); // Retry to connect to Wi-Fi
    }

    if (millis() - previousMillis >= 15000)
    {
        previousMillis = millis();
        OldCardID = "";
    }

    // Look for a new card
    if (!mfrc522.PICC_IsNewCardPresent())
    {
        digitalWrite(LED_PIN, LOW); // Turn off LED when no card is present
        return;                     // If no card is present, continue looping
    }

    // Select one of the cards
    if (!mfrc522.PICC_ReadCardSerial())
    {
        digitalWrite(LED_PIN, LOW); // Turn off LED if card read fails
        return;                     // If card read failed, return
    }

    String CardID = "";
    for (byte i = 0; i < mfrc522.uid.size; i++)
    {
        CardID += String(mfrc522.uid.uidByte[i], HEX); // Convert to HEX format
    }

    CardID.toUpperCase(); // Convert to uppercase for consistency

    // Debugging: Print the Card ID in the serial monitor
    Serial.print("Scanned Card UID: ");
    Serial.println(CardID);

    // Blink LED when card is detected
    // digitalWrite(LED_PIN, HIGH); // Turn on LED when card is detected

    if (CardID == OldCardID)
    {
        Serial.println("CARD MATCHED");
        return;
    }
    else
    {
        OldCardID = CardID;
        Serial.println("CARD ENROLLED");
    }

    Serial.print("Card ID: ");
    Serial.println(CardID);
    digitalWrite(LED_PIN, HIGH);

    SendCardID(CardID); // Send Card ID to the server
    delay(1000);        // Short delay to prevent multiple reads of the same card

    // Keep the LED on for a moment before turning it off
    delay(500);                 // Keep LED on for 500ms
    digitalWrite(LED_PIN, LOW); // Turn off LED
}
//************ Send the Card UID to the Website *************
void SendCardID(String Card_uid)
{
    Serial.println("Sending the Card ID");

    if (WiFi.status() == WL_CONNECTED)
    {
        HTTPClient http; // Declare HTTPClient object

        // Prepare GET Data
        getData = "?card_uid=" + Card_uid + "&device_token=" + String(device_token);
        Link = URL + getData;

        http.begin(client, Link);          // Initialize the HTTP request
        int httpCode = http.GET();         // Send GET request
        String payload = http.getString(); // Get response

        Serial.print("HTTP Response Code: ");
        Serial.println(httpCode);
        Serial.print("Card ID: ");
        Serial.println(Card_uid);
        Serial.print("Server Response: ");
        Serial.println(payload);

        http.end(); // Close HTTP connection
    }
    else
    {
        Serial.println("WiFi Not Connected! Failed to send Card ID");
    }
}
//******************** Connect to Wi-Fi ******************
void connectToWiFi()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("Already connected to Wi-Fi");
        return; // Already connected, no need to reconnect
    }

    WiFi.mode(WIFI_STA); // Set Wi-Fi to station mode
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print("."); // Indicate progress while connecting
    }

    Serial.println("\nConnected to Wi-Fi");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP()); // Print ESP IP address
    delay(1000);                    // Short delay after connection
}
