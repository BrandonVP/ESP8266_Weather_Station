/*
 Name:		ESP8266_Weather_Station.ino
 Created:	2/26/2021 1:31:59 PM
 Author:	brand
*/

#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266mDNS.h>
#include <Ticker.h>  //Ticker Library
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

#define KEL_TO_FAHRENHEIT(kelvin) (((kelvin - 273.15) * 1.8) + 32)
#define CEL_TO_FAHRENHEIT(CELSIUS) ((CELSIUS * 1.8) + 32)
#define BAUD_RATE 115200
#define MAX_CITY_LENGTH 20
#define DHTPIN 0 
#define UPDATE_INTERVAL 20
#define LCD_COLUMNS 20
#define LCD_ROWS 4
#define I2C_ADDRESS 0x27
#define JSON_BUFFER 5000

// Setup Temerature sensor
DHT dht(DHTPIN, DHT11);
int8_t insideHumidity;
int8_t insideTemperature;

// Setup I2C LCD
LiquidCrystal_I2C lcd(I2C_ADDRESS, LCD_COLUMNS, LCD_ROWS);

// WiFi Login Information
const char* ssid = "YOURWIFI";
const char* password = "YOUPASSWORD";

// Open Weather API URL and Key
const String URL = "Your API Key";

// Set true by system ticker
bool getWeather = false;

// Structure for local weather
struct openWeatherMap {
    char city[MAX_CITY_LENGTH];
    int8_t temperature;
    int8_t realFeel;
    int8_t tempHigh;
    int8_t tempLow;
    uint8_t humidity;
};
struct openWeatherMap localWeather;

// ArduinoJson
String json;
DynamicJsonDocument doc(JSON_BUFFER);

// System ticker object
Ticker weatherTimer;

// This function is called by system ticker to enable weather to update set by "UPDATE_INTERVAL"
void timerCall()
{
    getWeather = true;
}

// Get readings from DHT temperature sensor
void insideTemp()
{
    insideHumidity = dht.readHumidity();
    insideTemperature = dht.readTemperature();
}

// Updates LCD with newest readings
void updateLCD()
{
    lcd.setCursor(0, 1);
    lcd.print("");
    lcd.setCursor(11, 1);
    lcd.print((int8_t)CEL_TO_FAHRENHEIT(insideTemperature));
    lcd.setCursor(17, 1);
    lcd.print(insideHumidity);

    lcd.setCursor(11, 3);
    lcd.print(localWeather.temperature);
    lcd.setCursor(17, 3);
    lcd.print(localWeather.humidity);
}

// Requests local weather from Open Weather Map
void updateWeather()
{
    if ((WiFi.status() == WL_CONNECTED))
    {
        HTTPClient http;
        http.begin(URL);
        int httpCode = http.GET();
        if (httpCode > 0)
        {
            json = http.getString();
            Serial.println(json);
        }
        else
        {
            Serial.println("Error on HTTP request");
        }
        http.end();
    }
    // Parse incoming Json data
    DeserializationError error = deserializeJson(doc, json);
    if (error)
    {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
    }
    localWeather.temperature = KEL_TO_FAHRENHEIT((int8_t)doc["main"]["temp"]);
    localWeather.tempHigh = KEL_TO_FAHRENHEIT((int8_t)doc["main"]["temp_max"]);
    localWeather.tempLow = KEL_TO_FAHRENHEIT((int8_t)doc["main"]["temp_min"]);
    localWeather.humidity = doc["main"]["humidity"];

    getWeather = false;
}

// Initialize hardware
void setup() {
    Serial.begin(BAUD_RATE);

    // Initialize LCD
    lcd.init();                   
    lcd.backlight();

    // Initialize Temperature Sensor
    dht.begin();

    // Initialize WiFi
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.println("Connecting to WiFi..");
    }
    Serial.println("Connected to the WiFi network");

    // Attach system ticker 
    weatherTimer.attach(UPDATE_INTERVAL, timerCall);

    // Get and print starting values
    updateWeather();
    insideTemp();
    lcd.setCursor(2, 0);
    lcd.print("Weather Station");
    lcd.setCursor(0, 1);
    lcd.print(" Inside  T:    H:");
    lcd.setCursor(0, 3);
    lcd.print(" Outside T:    H:");
    updateLCD();
}

// Main loop
void loop()
{
    // Controlled by system ticker
    if (getWeather)
    {
        updateWeather();
        insideTemp();
        updateLCD();
    }
}
