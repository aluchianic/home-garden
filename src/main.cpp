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
// #include <LiquidCrystal_I2C.h>

// Custom
#include "pump.cpp"

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

// NodeMCU GPIO to D
int boardPins[4]{5, 4, 0, 2};

// Pump
Pump pump1;

// Potentionmeter
const int pt_analogInPin = A0; // ESP8266 Analog Pin ADC0 = A0
int pt_value = 1;              // value read from the pot
int pt_on = 0;                 // pot top right
int pt_off = 1024;             // pot top

// ---=== Tasks by interval ===----
struct task
{
    unsigned long rate;
    unsigned long previous;
};

task taskDashboard = {.rate = 500, .previous = 0};
task taskCheckTimeForPump = {.rate = 60000, .previous = 0};

void configUpdater()
{
    pump1.timeStart = PumpTime(configManager.data.wateringStartTime);
    pump1.timeStop = PumpTime(configManager.data.wateringStopTime);

    if (configManager.data.pumpSwitcher)
    {
        pump1.start();
    }
    if (!configManager.data.pumpSwitcher)
    {
        pump1.stop();
    }

    if (DEBUG)
    {
        Serial.println("Config changed, updating pump1 timer values.");

        Serial.print("pump1switcher is now: ");
        Serial.println(configManager.data.pumpSwitcher);

        Serial.print("pump1.timeStart : ");
        Serial.print(pump1.timeStart.hour);
        Serial.print(":");
        Serial.println(pump1.timeStart.min);

        Serial.print("pump1.timeStart : ");
        Serial.print(pump1.timeStop.hour);
        Serial.print(":");
        Serial.println(pump1.timeStop.min);
    }
}

void setup()
{
    Serial.begin(9600);

    LittleFS.begin();
    GUI.begin();
    configManager.begin();
    configManager.setConfigSaveCallback(configUpdater);
    WiFiManager.begin(configManager.data.projectName);
    timeSync.begin();
    dash.begin(500);

    timeSync.begin(TZ_Europe_Chisinau);
    //Wait for connection
    timeSync.waitForSyncResult(10000);

    if (timeSync.isSynced())
    {
        time_t now = time(nullptr);

        if (DEBUG)
        {
            Serial.print(PSTR("Time is set for Chisinau: "));
            Serial.print(asctime(localtime(&now)));
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

    pump1.initPump(
        boardPins[0],
        &dash.data.pump1status,
        configManager.data.wateringStartTime,
        configManager.data.wateringStopTime);

    if (digitalRead(boardPins[0]) == 0)
    {
        pump1.stop();
    }
}

void loop()
{
    //IOT lib software interrupts
    WiFiManager.loop();
    updater.loop();
    configManager.loop();
    dash.loop();

    // Current time
    time_t now = time(nullptr);
    struct tm *timeinfo;
    timeinfo = localtime(&now);

    // LCD set initial cursor
    // lcd.setCursor(0, 0);

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

        dash.data.humi = event.relative_humidity;
        dash.data.humidity = event.relative_humidity;
    }

    // ---=== Dashboard tasks ===---
    if (taskDashboard.previous == 0 || (millis() - taskDashboard.previous > taskDashboard.rate))
    {
        taskDashboard.previous = millis();

        String stringOne = "tomatoes";
        stringOne.toCharArray(dash.data.projectName, 32);

        //  pt - Potentiometer pump-1 manual ON/OFF
        pt_value = analogRead(pt_analogInPin);
        if (pt_value == pt_on)
        {
            pump1.start();
        }
        else if (pt_value == pt_off)
        {
            pump1.stop();
        }
    }

    // ---=== Pump tasks ===---
    if (taskCheckTimeForPump.previous == 0 || (millis() - taskCheckTimeForPump.previous > taskCheckTimeForPump.rate))
    {
        if (DEBUG)
        {
            Serial.print("time to start = ");
            Serial.print(pump1.timeStart.hour);
            Serial.print(":");
            Serial.print(pump1.timeStart.min);
            Serial.print("   -------     ");

            Serial.print("time to stop =");
            Serial.print(pump1.timeStop.hour);
            Serial.print(":");
            Serial.println(pump1.timeStop.min);
        }

        if (timeinfo->tm_hour == pump1.timeStart.hour && timeinfo->tm_min == pump1.timeStart.min)
        {
            pump1.start();
        }

        if (timeinfo->tm_hour == pump1.timeStop.hour && timeinfo->tm_min == pump1.timeStop.min)
        {
            pump1.stop();
        }
    }
}
