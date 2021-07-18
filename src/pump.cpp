#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

// C++
#include <string>

// Timer struct used control Pump start/stop
class PumpTime
{
public:
    int hour;
    int min;

    PumpTime()
    {
        Serial.println("PUMP default constructor were called");
    }

    PumpTime(char str[])
    {
        char *v[2];
        char *p;
        int i = 0;
        p = strtok(str, " ");
        while (p && i < 2)
        {
            v[i] = p;
            p = strtok(NULL, " ");
            i++;
        };

        PumpTime::hour = v[0] ? String(v[0]).toInt() : 0;
        PumpTime::min = v[1] ? String(v[1]).toInt() : 0;
    };
};

class Pump
{
public:
    int pin;
    bool flowing;
    bool dashboardUpdater;
    PumpTime timeStart;
    PumpTime timeStop;

    Pump()
    {
        Serial.println("PUMP default constructor were called");
    };

    void initPump(
        int p,
        bool *dashUpd,
        char startTime[],
        char stopTime[])
    {
        pin = p;
        dashboardUpdater = dashUpd;
        // Pump setup
        // PIN initialization for relay
        pinMode(pin, OUTPUT);

        timeStart = PumpTime(startTime);
        timeStop = PumpTime(stopTime);

        Serial.println("inited pump wit data:");
        Serial.println(" pin : ");
        Serial.println(pin);
        Serial.println("  dashboardUpdater : ");
        Serial.println(dashboardUpdater);
        Serial.println("  timeStart.hour : ");
        Serial.println(timeStart.hour);
        Serial.println("  timeStart.min : ");
        Serial.println(timeStart.min);
        Serial.println("  timeStop.hour : ");
        Serial.println(timeStop.hour);
        Serial.println("  timeStop.min : ");
        Serial.println(timeStop.min);
    }

    void start()
    {
        digitalWrite(pin, LOW);

        flowing = true;
        dashboardUpdater = true;

        Serial.println("Pump started");
    };

    void stop()
    {
        digitalWrite(pin, HIGH);

        flowing = false;
        dashboardUpdater = false;

        Serial.println("Pump stoped");
    };
};