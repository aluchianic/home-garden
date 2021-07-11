#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

#include "LittleFS.h"

#include "WiFiManager.h"
#include "webServer.h"
#include "updater.h"
#include "configManager.h"
#include "dashboard.h"
#include "timeSync.h"

#include <string>

// DHT
#include <DHT.h>
#include <DHT_U.h>

// LCD
#include <LiquidCrystal_I2C.h>

// DHT setup
#define DHTPIN 2 // Digital pin connected to the DHT sensor
// Feather HUZZAH ESP8266 note: use pins 3, 4, 5, 12, 13 or 14 --
// Pin 15 can work but DHT must be disconnected during program upload.

// Uncomment the type of sensor in use:
#define DHTTYPE DHT11 // DHT 11
// #define DHTTYPE DHT22 // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)

// See guide for details on sensor wiring and usage:
//   https://learn.adafruit.com/dht/overview

DHT_Unified dht(DHTPIN, DHTTYPE);

uint32_t dhtDelayMS;

void configureDHT(sensor_t sensor)
{
    Serial.println(F("DHTxx Unified Sensor Example"));
    // Print temperature sensor details.
    dht.temperature().getSensor(&sensor);
    Serial.println(F("------------------------------------"));
    Serial.println(F("Temperature Sensor"));
    Serial.print(F("Sensor Type: "));
    Serial.println(sensor.name);
    Serial.print(F("Driver Ver:  "));
    Serial.println(sensor.version);
    Serial.print(F("Unique ID:   "));
    Serial.println(sensor.sensor_id);
    Serial.print(F("Max Value:   "));
    Serial.print(sensor.max_value);
    Serial.println(F("°C"));
    Serial.print(F("Min Value:   "));
    Serial.print(sensor.min_value);
    Serial.println(F("°C"));
    Serial.print(F("Resolution:  "));
    Serial.print(sensor.resolution);
    Serial.println(F("°C"));
    Serial.println(F("------------------------------------"));
    // Print humidity sensor details.
    dht.humidity().getSensor(&sensor);
    Serial.println(F("Humidity Sensor"));
    Serial.print(F("Sensor Type: "));
    Serial.println(sensor.name);
    Serial.print(F("Driver Ver:  "));
    Serial.println(sensor.version);
    Serial.print(F("Unique ID:   "));
    Serial.println(sensor.sensor_id);
    Serial.print(F("Max Value:   "));
    Serial.print(sensor.max_value);
    Serial.println(F("%"));
    Serial.print(F("Min Value:   "));
    Serial.print(sensor.min_value);
    Serial.println(F("%"));
    Serial.print(F("Resolution:  "));
    Serial.print(sensor.resolution);
    Serial.println(F("%"));
    Serial.println(F("------------------------------------"));

    // Set delay between sensor readings based on sensor details.
    dhtDelayMS = sensor.min_delay / 1000;
}

// LCD setup
// set the LCD number of columns and rows
int lcdColumns = 16;
int lcdRows = 2;

// set LCD address, number of columns and rows
// if you don't know your display address, run an I2C scanner sketch
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

String message = "";

// DEBUG - set to true for more information
bool DEBUG = false;

struct task
{
    unsigned long rate;
    unsigned long previous;
};

task taskA = {.rate = 500, .previous = 0};

void setup()
{
    Serial.begin(9600);

    LittleFS.begin();
    GUI.begin();
    configManager.begin();
    WiFiManager.begin(configManager.data.projectName);
    timeSync.begin();
    dash.begin(500);

    // Initialize DHT device.
    dht.begin();
    sensor_t sensor;
    configureDHT(sensor);

    // LCD initialize
    lcd.init();
    // turn on LCD backlight
    lcd.backlight();
}

void loop()
{
    //IOT lib software interrupts
    WiFiManager.loop();
    updater.loop();
    configManager.loop();
    dash.loop();

    // LCD set initial cursor
    lcd.setCursor(0, 0);

    //DHT delay for reading sensor events
    delay(dhtDelayMS);
    // Get temperature event and print its value.
    sensors_event_t event;
    dht.temperature().getEvent(&event);
    if (isnan(event.temperature))
    {
        Serial.println(F("Error reading temperature!"));
    }
    else
    {
        if (DEBUG)
        {
            Serial.print(F("Temperature: "));
            Serial.print(event.temperature);
            Serial.println(F("°C"));
        }

        // LCD set initial cursor on top line
        lcd.setCursor(0, 0);
        message = F("Temp: ") + String(event.temperature) + F(" C");
        lcd.print(message);
        dash.data.temp = event.temperature;
        dash.data.temperature = event.temperature;
    }
    // Get humidity event and print its value.
    dht.humidity().getEvent(&event);
    if (isnan(event.relative_humidity))
    {
        Serial.println(F("Error reading humidity!"));
    }
    else
    {
        if (DEBUG)
        {
            Serial.print(F("Humidity: "));
            Serial.print(event.relative_humidity);
            Serial.println(F("%"));
        }

        // LCD set initial cursor on bottom line
        lcd.setCursor(0, 1);
        message = F("Hum: ") + String(event.relative_humidity) + F("%");
        lcd.print(message);
        dash.data.humi = event.relative_humidity;
        dash.data.humidity = event.relative_humidity;
    }

    // ---=== Dashboard ===---
    if (taskA.previous == 0 || (millis() - taskA.previous > taskA.rate))
    {
        taskA.previous = millis();

        String stringOne = "tomatoes";
        stringOne.toCharArray(dash.data.projectName, 32);
    }
}
