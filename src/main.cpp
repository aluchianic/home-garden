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

// Timezones
#include <TZ.h>

// DHT
#include <DHT.h>
#include <DHT_U.h>

// LCD
#include <LiquidCrystal_I2C.h>

// DEBUG - set to true for more information
#define DEBUG false

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

// Current Time
struct tm *tm_struct;

// NodeMCU GPIO to D
int boardPins[4]{5, 4, 0, 2};

// Pump
struct Pump
{
    int initial;
    bool flowing;
    int timeInMin;
    int startTime;
};

Pump pump1;

void stopPump(Pump pump)
{
    pump.flowing = false;
    digitalWrite(boardPins[0], HIGH);

    if (DEBUG)
    {
        Serial.println("Pump stoped");
    }
}

void startPump(Pump pump)
{
    pump.flowing = true;
    digitalWrite(boardPins[0], LOW);

    if (DEBUG)
    {
        Serial.println("Pump started");
    }
}

// Potentionmeter
const int pt_analogInPin = A0; // ESP8266 Analog Pin ADC0 = A0
int pt_value = 0;              // value read from the pot
int pt_on = 5;                 // pot top right
int pt_off = 1024;             // pot top

// LCD setup
// set the LCD number of columns and rows
int lcdColumns = 16;
int lcdRows = 2;

// set LCD address, number of columns and rows
// if you don't know your display address, run an I2C scanner sketch
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

String message = "";

// ---=== Tasks by interval ===----
struct task
{
    unsigned long rate;
    unsigned long previous;
};

task taskDashboard = {.rate = 500, .previous = 0};
task taskCheckTimeForPump = {.rate = 60000, .previous = 0};

void setup()
{
    Serial.begin(9600);

    LittleFS.begin();
    GUI.begin();
    configManager.begin();
    WiFiManager.begin(configManager.data.projectName);
    timeSync.begin();
    dash.begin(500);

    timeSync.begin(TZ_Europe_Chisinau);
    //Wait for connection
    timeSync.waitForSyncResult(10000);

    if (timeSync.isSynced())
    {
        time_t now = time(nullptr);
        struct tm *tm_struct = localtime(&now);

        if (DEBUG)
        {
            Serial.print(PSTR("Time is set for Chisinau: "));
            Serial.print(asctime(tm_struct));
        }
    }
    else
    {
        Serial.println("Timeout while receiving the time");
    }

    // Initialize DHT device.
    dht.begin();
    sensor_t sensor;
    configureDHT(sensor);

    // LCD initialize
    lcd.init();
    // turn on LCD backlight
    lcd.backlight();

    // Pump setup
    // PIN initialization for relay
    pinMode(boardPins[0], OUTPUT);
    // Keep pin1 OFF on setup

    Pump pump1 = {
        digitalRead(boardPins[0]),
        false,
        configManager.data.wateringDuration,
        configManager.data.wateringStartTime};

    if (pump1.initial == 0)
    {
        stopPump(pump1);
    }
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

    // Potentiometer pump-1 manual ON/OFF
    pt_value = analogRead(pt_analogInPin);
    if (pt_value == pt_off)
    {
        stopPump(pump1);
    }
    if (pt_value == pt_on)
    {
        startPump(pump1);
    }

    // ---=== Dashboard tasks ===---
    if (taskDashboard.previous == 0 || (millis() - taskDashboard.previous > taskDashboard.rate))
    {
        taskDashboard.previous = millis();

        String stringOne = "tomatoes";
        stringOne.toCharArray(dash.data.projectName, 32);

        dash.data.pump1 = pump1.flowing;
    }

    // ---=== Pump tasks ===---
    if (taskCheckTimeForPump.previous == 0 || (millis() - taskCheckTimeForPump.previous > taskCheckTimeForPump.rate))
    {
        taskCheckTimeForPump.previous = millis();
        if (tm_struct->tm_hour == pump1.startTime)
        {
            if (tm_struct->tm_min == 0)
            {
                startPump(pump1);
            }
            if (tm_struct->tm_min >= pump1.timeInMin)
            {
                stopPump(pump1);
            }
        }
        // else
        // {
        //     // ensure that pump1 is off
        //     if (pump1.flowing == true)
        //     {
        //         stopPump(pump1);
        //     }
        // }
    }
}
