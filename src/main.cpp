//----------------------------------------Including the libraries.
#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include "WiFi.h"
#include <HTTPClient.h>
//----------------------------------------

// Defines SS/SDA PIN and Reset PIN for RFID-RC522.
#define SS_PIN 5
#define RST_PIN 4

//----------------------------------------SSID and PASSWORD of your WiFi network.
const char *ssid = "BedDH_Fibernet"; //--> Your wifi name
const char *password = "AR27@69MY";  //--> Your wifi password
//----------------------------------------

// Google script Web_App_URL.
String Web_App_URL = "https://script.google.com/macros/s/AKfycbx2UIrgwV3ih8jL7ee1e8YBGWIyXPKswORViJwYgmf-2TWvtc_pzn3hro7mWWz68ZlOAQ/exec";

String reg_Info = "";

String atc_Info = "";
String atc_Name = "";
String atc_Date = "";
String atc_Time_In = "";
String atc_Time_Out = "";

// Variables for the number of columns and rows on the LCD.
int lcdColumns = 20;
int lcdRows = 4;

// Variable to read data from RFID-RC522.
int readsuccess;
char str[32] = "";
String UID_Result = "--------";

String modes = "atc";

// Create MFRC522 object as "mfrc522" and set SS/SDA PIN and Reset PIN.
MFRC522 mfrc522(SS_PIN, RST_PIN); //--> Create MFRC522 instance.

// Function Prototypes
String getValue(String data, char separator, int index);                 // Function prototype for getValue
void byteArray_to_string(byte array[], unsigned int len, char buffer[]); // Function prototype for byteArray_to_string

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

//________________________________________________________________________________http_Req()
// Subroutine for sending HTTP requests to Google Sheets.
void http_Req(String str_modes, String str_uid)
{
    if (WiFi.status() == WL_CONNECTED)
    {
        String http_req_url = "";

        //----------------------------------------Create links to make HTTP requests to Google Sheets.
        if (str_modes == "atc")
        {
            http_req_url = Web_App_URL + "?sts=atc";
            http_req_url += "&uid=" + str_uid;
        }
        if (str_modes == "reg")
        {
            http_req_url = Web_App_URL + "?sts=reg";
            http_req_url += "&uid=" + str_uid;
        }
        //----------------------------------------

        //----------------------------------------Sending HTTP requests to Google Sheets.
        Serial.println();
        Serial.println("-------------");
        Serial.println("Sending request to Google Sheets...");
        Serial.print("URL : ");
        Serial.println(http_req_url);

        // Create an HTTPClient object as "http".
        HTTPClient http;

        // HTTP GET Request.
        http.begin(http_req_url.c_str());
        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

        // Gets the HTTP status code.
        int httpCode = http.GET();
        Serial.print("HTTP Status Code : ");
        Serial.println(httpCode);

        // Getting response from google sheet.
        String payload;
        if (httpCode > 0)
        {
            payload = http.getString();
            Serial.println("Payload : " + payload);
        }

        Serial.println("-------------");
        http.end();
        //----------------------------------------

        String sts_Res = getValue(payload, ',', 0);

        //----------------------------------------Conditions that are executed are based on the payload response from Google Sheets (the payload response is set in Google Apps Script).
        if (sts_Res == "OK")
        {
            if (str_modes == "atc")
            {
                atc_Info = getValue(payload, ',', 1);

                if (atc_Info == "TI_Successful")
                {
                    atc_Name = getValue(payload, ',', 2);
                    atc_Date = getValue(payload, ',', 3);
                    atc_Time_In = getValue(payload, ',', 4);

                    Serial.println("Attendance In:");
                    Serial.println("Name: " + atc_Name);
                    Serial.println("Date: " + atc_Date);
                    Serial.println("Time In: " + atc_Time_In);
                    delay(5000);
                }

                if (atc_Info == "TO_Successful")
                {
                    atc_Name = getValue(payload, ',', 2);
                    atc_Date = getValue(payload, ',', 3);
                    atc_Time_In = getValue(payload, ',', 4);
                    atc_Time_Out = getValue(payload, ',', 5);

                    Serial.println("Attendance Out:");
                    Serial.println("Name: " + atc_Name);
                    Serial.println("Date: " + atc_Date);
                    Serial.println("Time In: " + atc_Time_In);
                    Serial.println("Time Out: " + atc_Time_Out);
                    delay(5000);
                }

                if (atc_Info == "atcInf01")
                {
                    Serial.println("You have completed your attendance for today.");
                    delay(5000);
                }

                if (atc_Info == "atcErr01")
                {
                    Serial.println("Error: Your card or keychain is not registered.");
                    delay(5000);
                }

                atc_Info = "";
                atc_Name = "";
                atc_Date = "";
                atc_Time_In = "";
                atc_Time_Out = "";
            }

            if (str_modes == "reg")
            {
                reg_Info = getValue(payload, ',', 1);

                if (reg_Info == "R_Successful")
                {
                    Serial.println("UID successfully uploaded.");
                    delay(5000);
                }

                if (reg_Info == "regErr01")
                {
                    Serial.println("Error: The UID of your card or keychain has been registered.");
                    delay(5000);
                }

                reg_Info = "";
            }
        }
    }
    else
    {
        Serial.println("WiFi disconnected.");
        delay(3000);
    }
}
//________________________________________________________________________________

//________________________________________________________________________________getValue()
// String function to process the data (Split String).
String getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = {0, -1};
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++)
    {
        if (data.charAt(i) == separator || i == maxIndex)
        {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i + 1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

//________________________________________________________________________________getUID()
// Function to get the UID from the RFID card.
int getUID()
{
    if (!mfrc522.PICC_IsNewCardPresent())
    {
        return 0;
    }
    if (!mfrc522.PICC_ReadCardSerial())
    {
        return 0;
    }

    byteArray_to_string(mfrc522.uid.uidByte, mfrc522.uid.size, str);
    UID_Result = str;

    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();

    return 1;
}

//________________________________________________________________________________byteArray_to_string()
// Convert byte array to string.
void byteArray_to_string(byte array[], unsigned int len, char buffer[])
{
    for (unsigned int i = 0; i < len; i++)
    {
        byte nib1 = (array[i] >> 4) & 0x0F;
        byte nib2 = (array[i] >> 0) & 0x0F;
        buffer[i * 2 + 0] = nib1 < 0xA ? '0' + nib1 : 'A' + nib1 - 0xA;
        buffer[i * 2 + 1] = nib2 < 0xA ? '0' + nib2 : 'A' + nib2 - 0xA;
    }
    buffer[len * 2] = '\0';
}

//________________________________________________________________________________setup()
// Setup the WiFi and RFID
void setup()
{
    // Start Serial communication
    Serial.begin(115200);

    // Connect to WiFi

    delay(1000);
    Serial.println("Connecting to WiFi...");
    connectToWiFi();

    Serial.println("Connected to WiFi!");

    // Initialize the MFRC522 RFID reader
    SPI.begin();
    mfrc522.PCD_Init();
    Serial.println("Scan a card...");

    // Delay to ensure everything is initialized before loop
    delay(1000);
}

//________________________________________________________________________________loop()
// Main loop that reads the RFID card and makes HTTP requests.
void loop()
{
    if (getUID())
    {
        // Print the UID to serial
        Serial.println("Card UID: " + UID_Result);

        // Send the UID to the Google Sheets script for attendance
        http_Req(modes, UID_Result);

        // Add a small delay before reading the card again
        delay(2000);
    }
}
