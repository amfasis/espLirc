#include "wifi.h"
#include "FS.h"
#include "config.h"

bool connect_wifi()
{
    File configF = SPIFFS.open(path_wifi_txt, "r");
    if(!configF)
        return false; //should reboot to config

    WifiConfig config;
    configF.readBytes((char*)&config, sizeof(config));
    configF.close();

    if(config.ssid[0] == 0)
        return false;

    IPAddress ip(config.ip[0], config.ip[1], config.ip[2], config.ip[3]);
    IPAddress gateway(config.gateway[0], config.gateway[1], config.gateway[2], config.gateway[3]);
    IPAddress subnet(config.netmask[0], config.netmask[1], config.netmask[2], config.netmask[3]);

    Serial.setDebugOutput(false);

    for (int i = 0; i < 3 && WiFi.status() != WL_CONNECTED; i++)
    {
        Serial.print("begin wifi to ");
        Serial.println(config.ssid);

        WiFi.forceSleepWake();
        delay(1);
        
        // Disable the WiFi persistence.  The ESP8266 will not load and save WiFi settings in the flash memory.
        WiFi.persistent( false );

        WiFi.mode( WIFI_STA );

        if(!(config.ip[0] == 0 && config.ip[1] == 0 && config.ip[2] == 0 && config.ip[3] == 0))
        {
            Serial.println("Using static ip");
            WiFi.config(ip, gateway, subnet);
        }
        WiFi.begin(config.ssid, config.pass);

        for (int j = 0; j < 30 && WiFi.status() != WL_CONNECTED; j++)
        {
            Serial.println("waiting for Wifi");
            digitalWrite(LED_BUILTIN, LOW); // Turn the LED on (Note that LOW is the voltage level)
            delay(100);
            digitalWrite(LED_BUILTIN, HIGH); // Turn the LED off
            delay(400);
        }

        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.println("wifi ok");
            return true;
        }

        Serial.setDebugOutput(true); //if at first it doesn't properly connect, try with debug

        Serial.print("Wifi status:");
        Serial.println(WiFi.status());

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