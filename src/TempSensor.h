#ifndef TempSensor_h
#define TempSensor_h

#include <OneWire.h>
#include <DallasTemperature.h>

String discoverOneWireDevices();

OneWire ds(2); // on pin 2 (a 4.7K pullup is necessary)
DallasTemperature sensors(&ds);

void initOneWire()
{
    sensors.begin();
    // discoverOneWireDevices();
}

String getTemperatur()
{
    sensors.requestTemperatures();
    return (String)sensors.getTempCByIndex(0);
    // return "23.5";
}

String discoverOneWireDevices(void)
{
    byte i;
    byte present = 0;
    byte data[12];
    byte addr[8];
    String found = "";

    Serial.print("Looking for 1-Wire devices...\n\r");
    while (ds.search(addr))
    {
        found = "gefunden";
        Serial.print("\n\rFound \'1-Wire\' device with address:\n\r");
        for (i = 0; i < 8; i++)
        {
            Serial.print("0x");
            if (addr[i] < 16)
            {
                Serial.print('0');
            }
            Serial.print(addr[i], HEX);
            if (i < 7)
            {
                Serial.print(", ");
            }
        }
        if (OneWire::crc8(addr, 7) != addr[7])
        {
            Serial.print("CRC is not valid!\n");
            return found;
        }
    }
    Serial.print("\n\r\n\rThat's it.\r\n");
    ds.reset_search();
    return found;
}

#endif