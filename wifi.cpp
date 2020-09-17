#include "wifi.h"
#include <FS.h>
#include "config.h"
#include "print.h"

bool connect_wifi()
{
    if(configWifi.ssid[0] == 0)
        return false;

    IPAddress ip(configWifi.ip[0], configWifi.ip[1], configWifi.ip[2], configWifi.ip[3]);
    IPAddress gateway(configWifi.gateway[0], configWifi.gateway[1], configWifi.gateway[2], configWifi.gateway[3]);
    IPAddress subnet(configWifi.netmask[0], configWifi.netmask[1], configWifi.netmask[2], configWifi.netmask[3]);

    Serial.setDebugOutput(false);

    for (int i = 0; i < 3 && WiFi.status() != WL_CONNECTED; i++)
    {
        PRINTF("begin wifi to ");
        PRINTLN(configWifi.ssid);

        WiFi.forceSleepWake();
        delay(1);
        
        // Disable the WiFi persistence.  The ESP8266 will not load and save WiFi settings in the flash memory.
        WiFi.persistent( false );

        WiFi.mode( WIFI_STA );

        if(!(configWifi.ip[0] == 0 && configWifi.ip[1] == 0 && configWifi.ip[2] == 0 && configWifi.ip[3] == 0))
        {
            PRINTLN("Using static ip");
            WiFi.config(ip, gateway, subnet);
        }
        WiFi.begin(configWifi.ssid, configWifi.pass);

        for (int j = 0; j < 30 && WiFi.status() != WL_CONNECTED; j++)
        {
            PRINTLN("waiting for Wifi");
            digitalWrite(LED_BUILTIN, LOW); // Turn the LED on (Note that LOW is the voltage level)
            delay(100);
            digitalWrite(LED_BUILTIN, HIGH); // Turn the LED off
            delay(400);
        }

        if (WiFi.status() == WL_CONNECTED)
        {
            PRINTLN("wifi ok");
            return true;
        }

        Serial.setDebugOutput(true); //if at first it doesn't properly connect, try with debug

        PRINTF("Wifi status:");
        PRINTLN(WiFi.status());

        for (int i = 0; i < WiFi.status(); i++)
        {
            digitalWrite(LED_BUILTIN, LOW); // Turn the LED on (Note that LOW is the voltage level)
            delay(450);
            digitalWrite(LED_BUILTIN, HIGH); // Turn the LED off
            delay(50);
        }

        //    WiFi.disconnect();

        delay(300);
        //blink 1 seconds
        for (int j = 0; j < 33 && WiFi.status() != WL_CONNECTED; j++)
        {
            digitalWrite(LED_BUILTIN, LOW); // Turn the LED on (Note that LOW is the voltage level)
            delay(15);
            digitalWrite(LED_BUILTIN, HIGH); // Turn the LED off
            delay(15);
        }

        delay(10000); //wait 10 seconds before new try
    }

    Serial.setDebugOutput(false);

    return WiFi.status() == WL_CONNECTED;
}