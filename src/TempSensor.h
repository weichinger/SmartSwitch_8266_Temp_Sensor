#ifndef TempSensor_h
#define TempSensor_h

#include <OneWire.h>

const int nsensors = 2;
byte sensors[][8] = {
    {0x28, 0xC1, 0x02, 0x64, 0x04, 0x00, 0x00, 0x35},
    {0x28, 0xE7, 0x0B, 0x63, 0x04, 0x00, 0x00, 0x44}};
int16_t tempraw[nsensors];
unsigned long nextprint = 0;

OneWire ds(2); // on pin 2 (a 4.7K pullup is necessary)

void setup()
{
    Serial.begin(115200);
    delay(10);

    Serial.println();
    Serial.println();
}

void loop()
{
    ds18process(); // call this every loop itteration, the more calls the better.

    if (millis() > nextprint)
    {
        Serial.print("Temp F: ");
        for (byte j = 0; j < nsensors; j++)
        {
            Serial.print(j);
            Serial.print("=");
            Serial.print(ds18temp(1, tempraw[j]));
            Serial.print("  ");
        }
        Serial.println();
        nextprint = millis() + 1000; // print once a second
    }
}

/* Process the sensor data in stages.
 * each stage will run quickly. the conversion
 * delay is done via a millis() based delay.
 * a 5 second wait between reads reduces self
 * heating of the sensors.
 */
void ds18process()
{
    static byte stage = 0;
    static unsigned long timeNextStage = 0;
    static byte sensorindex = 100;
    byte i, j;
    byte present = 0;
    byte type_s;
    byte data[12];
    byte addr[8];

    if (stage == 0 && millis() > timeNextStage)
    {
        if (!ds.search(addr))
        {
            // no more, reset search and pause
            ds.reset_search();
            timeNextStage = millis() + 5000; // 5 seconds until next read
            return;
        }
        else
        {
            if (OneWire::crc8(addr, 7) != addr[7])
            {
                Serial.println("CRC is not valid!");
                return;
            }
            // got one, start stage 1
            stage = 1;
        }
    }
    if (stage == 1)
    {
        Serial.print("ROM =");
        for (i = 0; i < 8; i++)
        {
            Serial.write(' ');
            Serial.print(addr[i], HEX);
        }
        // find sensor
        for (j = 0; j < nsensors; j++)
        {
            sensorindex = j;
            for (i = 0; i < 8; i++)
            {
                if (sensors[j][i] != addr[i])
                {
                    sensorindex = 100;
                    break; // stop the i loop
                }
            }
            if (sensorindex < 100)
            {
                break; // found it, stop the j loop
            }
        }
        if (sensorindex == 100)
        {
            Serial.println("  Sensor not found in array");
            stage = 0;
            return;
        }
        Serial.print("  index=");
        Serial.println(sensorindex);

        ds.reset();
        ds.select(sensors[sensorindex]);
        ds.write(0x44, 0);               // start conversion, with parasite power off at the end
        stage = 2;                       // now wait for stage 2
        timeNextStage = millis() + 1000; // wait 1 seconds for the read
    }

    if (stage == 2 && millis() > timeNextStage)
    {
        // the first ROM byte indicates which chip
        switch (sensors[sensorindex][0])
        {
        case 0x10:
            Serial.print("  Chip = DS18S20"); // or old DS1820
            Serial.print("  index=");
            Serial.println(sensorindex);
            type_s = 1;
            break;
        case 0x28:
            Serial.print("  Chip = DS18B20");
            Serial.print("  index=");
            Serial.println(sensorindex);
            type_s = 0;
            break;
        case 0x22:
            Serial.print("  Chip = DS1822");
            Serial.print("  index=");
            Serial.println(sensorindex);
            type_s = 0;
            break;
        default:
            Serial.println("Device is not a DS18x20 family device.");
            stage = 0;
            return;
        }

        present = ds.reset();
        ds.select(sensors[sensorindex]);
        ds.write(0xBE); // Read Scratchpad

        Serial.print("  Data = ");
        Serial.print(present, HEX);
        Serial.print(" ");
        for (i = 0; i < 9; i++)
        { // we need 9 bytes
            data[i] = ds.read();
            Serial.print(data[i], HEX);
            Serial.print(" ");
        }
        Serial.print(" CRC=");
        Serial.print(OneWire::crc8(data, 8), HEX);
        Serial.print(" index=");
        Serial.print(sensorindex);
        Serial.println();

        int16_t raw = (data[1] << 8) | data[0];
        if (type_s)
        {
            raw = raw << 3; // 9 bit resolution default
            if (data[7] == 0x10)
            {
                // "count remain" gives full 12 bit resolution
                raw = (raw & 0xFFF0) + 12 - data[6];
            }
        }
        else
        {
            byte cfg = (data[4] & 0x60);
            // at lower res, the low bits are undefined, so let's zero them
            if (cfg == 0x00)
                raw = raw & ~7; // 9 bit resolution, 93.75 ms
            else if (cfg == 0x20)
                raw = raw & ~3; // 10 bit res, 187.5 ms
            else if (cfg == 0x40)
                raw = raw & ~1; // 11 bit res, 375 ms
                                //// default is 12 bit resolution, 750 ms conversion time
        }
        tempraw[sensorindex] = raw;
        stage = 0;
    }
}

/* Converts raw temp to Celsius or Fahrenheit
 * scale: 0=celsius, 1=fahrenheit
 * raw: raw temp from sensor
 *
 * Call at any time to get the last save temperature
 */
float ds18temp(byte scale, int16_t raw)
{
    switch (scale)
    {
    case 0: // Celsius
        return (float)raw / 16.0;
        break;
    case 1: // Fahrenheit
        return (float)raw / 16.0 * 1.8 + 32.0;
        break;
    default: // er, wut
        return -255;
    }
}

#endif