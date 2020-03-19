#include "setup_config.h"
#include "config.h"
#include <FS.h>
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>

#include "wifi.h"

ESP8266WebServer server(80);

void handleRoot()
{
    Serial.println("handling root");
    File configF = SPIFFS.open(path_wifi_txt, "r");
    WifiConfig configW;
    if(configF)
    {
        configF.readBytes((char*)&configW, sizeof(configW));
        configF.close();

        Serial.println(configW.ssid);
        Serial.println(configW.pass);
        Serial.print((int)configW.ip[0]);
        Serial.print((int)configW.ip[1]);
        Serial.print((int)configW.ip[2]);
        Serial.println((int)configW.ip[3]);
        Serial.print((int)configW.netmask[0]);
        Serial.print((int)configW.netmask[1]);
        Serial.print((int)configW.netmask[2]);
        Serial.println((int)configW.netmask[3]);
        Serial.print((int)configW.gateway[0]);
        Serial.print((int)configW.gateway[1]);
        Serial.print((int)configW.gateway[2]);
        Serial.println((int)configW.gateway[3]);
    }

    server.send(200, "text/html", "<html><body>\
    <h1>Wifi settings</h1>\
    <form method=\"post\" enctype=\"application/x-www-form-urlencoded\" action=\"/wifi/\">\
        <input type=\"text\" name=\"ssid\" placeholder=\"ssid\" maxlength=\"15\"/><br/>\
        <input type=\"text\" name=\"pass\" placeholder=\"pass\" maxlength=\"35\"/><br/>\
        <input type=\"text\" name=\"ip1\" placeholder=\"ip1\" min=\"0\" max=\"255\"/>\
        <input type=\"text\" name=\"ip2\" placeholder=\"ip2\" min=\"0\" max=\"255\"/>\
        <input type=\"text\" name=\"ip3\" placeholder=\"ip3\" min=\"0\" max=\"255\"/>\
        <input type=\"text\" name=\"ip4\" placeholder=\"ip4\" min=\"0\" max=\"255\"/><br/>\
        <input type=\"text\" name=\"nm1\" placeholder=\"nm1\" min=\"0\" max=\"255\"/>\
        <input type=\"text\" name=\"nm2\" placeholder=\"nm2\" min=\"0\" max=\"255\"/>\
        <input type=\"text\" name=\"nm3\" placeholder=\"nm3\" min=\"0\" max=\"255\"/>\
        <input type=\"text\" name=\"nm4\" placeholder=\"nm4\" min=\"0\" max=\"255\"/><br/>\
        <input type=\"text\" name=\"gw1\" placeholder=\"gw1\" min=\"0\" max=\"255\"/>\
        <input type=\"text\" name=\"gw2\" placeholder=\"gw2\" min=\"0\" max=\"255\"/>\
        <input type=\"text\" name=\"gw3\" placeholder=\"gw3\" min=\"0\" max=\"255\"/>\
        <input type=\"text\" name=\"gw4\" placeholder=\"gw4\" min=\"0\" max=\"255\"/><br/>\
        <input type=\"submit\" value=\"Opslaan\"/>\
    </form>\
    <h1>Lirc settings</h1>\
    <form method=\"post\" enctype=\"multipart/form-data\" action=\"/lirc/\">\
        <input type=\"file\" name=\"name\">\
        <input class=\"button\" type=\"submit\" value=\"Upload\">\
    </form>\
    <h1>Remove config</h1>\
    <form method=\"post\" enctype=\"application/x-www-form-urlencoded\" action=\"/clear/\">\
        <input type=\"submit\" value=\"Clear\"/>\
    </form>\
    <h1>Reboot</h1>\
    <form method=\"post\" enctype=\"application/x-www-form-urlencoded\" action=\"/reboot/\">\
        <input type=\"submit\" value=\"Reboot\"/>\
    </form>\
    </body></html>");
}

void handleWifiSettings()
{
    Serial.println("handling wifi settings");
    File configF = SPIFFS.open(path_wifi_txt, "r");
    WifiConfig config;
    if(configF)
    {
        configF.readBytes((char*)&config, sizeof(config));
        configF.close();
    }

    if(server.hasArg("ssid"))
        server.arg("ssid").toCharArray(config.ssid, sizeof(config.ssid));
    if(server.hasArg("pass"))
        server.arg("pass").toCharArray(config.pass, sizeof(config.pass));
    
    if(server.hasArg("ip1"))
        config.ip[0] = static_cast<uint8_t>(server.arg("ip1").toInt());
    if(server.hasArg("ip2"))
        config.ip[1] = static_cast<uint8_t>(server.arg("ip2").toInt());
    if(server.hasArg("ip3"))
        config.ip[2] = static_cast<uint8_t>(server.arg("ip3").toInt());
    if(server.hasArg("ip4"))
        config.ip[3] = static_cast<uint8_t>(server.arg("ip4").toInt());

    if(server.hasArg("gw1"))
        config.gateway[0] = static_cast<uint8_t>(server.arg("gw1").toInt());
    if(server.hasArg("gw2"))
        config.gateway[1] = static_cast<uint8_t>(server.arg("gw2").toInt());
    if(server.hasArg("gw3"))
        config.gateway[2] = static_cast<uint8_t>(server.arg("gw3").toInt());
    if(server.hasArg("gw4"))
        config.gateway[3] = static_cast<uint8_t>(server.arg("gw4").toInt());

    if(server.hasArg("nm1"))
        config.netmask[0] = static_cast<uint8_t>(server.arg("nm1").toInt());
    if(server.hasArg("nm2"))
        config.netmask[1] = static_cast<uint8_t>(server.arg("nm2").toInt());
    if(server.hasArg("nm3"))
        config.netmask[2] = static_cast<uint8_t>(server.arg("nm3").toInt());
    if(server.hasArg("nm4"))
        config.netmask[3] = static_cast<uint8_t>(server.arg("nm4").toInt());

    File configF2 = SPIFFS.open(path_wifi_txt, "w+");
    if(configF2)
    {
        unsigned char * data = reinterpret_cast<unsigned char*>(&config);
        size_t bw = configF2.write(data, sizeof(config));
        configF2.close();
        Serial.println(bw);
    }
    else
    {
        Serial.println("could not open wifi.txt");
    }

    if (server.method() != HTTP_POST) 
    {
        server.send(405, "text/plain", "Method Not Allowed");
    } 
    else 
    {
        server.sendHeader("Location", String("/"), true);
        server.send ( 302, "text/plain", "");
    }
}

File fsUploadFile;
void handleLircUpload()
{
    Serial.println("Handling lirc upload");

    // upload a new file to the SPIFFS
    HTTPUpload &upload = server.upload();
    if (upload.status == UPLOAD_FILE_START)
    {
        String filename = upload.filename;
        if (!filename.startsWith("/"))
            filename = "/" + filename;
        Serial.print("handleFileUpload Name: ");
        Serial.println(filename);
        fsUploadFile = SPIFFS.open(path_lirc_txt, "w"); // Open the file for writing in SPIFFS (create if it doesn't exist)
        filename = String();
    }
    else if (upload.status == UPLOAD_FILE_WRITE)
    {
        if (fsUploadFile)
            fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
    }
    else if (upload.status == UPLOAD_FILE_END)
    {
        if (fsUploadFile)
        {                         // If the file was successfully created
            fsUploadFile.close(); // Close the file again
            Serial.print("handleFileUpload Size: ");
            Serial.println(upload.totalSize);
            server.sendHeader("Location", "/"); // Redirect the client to the success page
            server.send(303);
        }
        else
        {
            server.send(500, "text/plain", "500: couldn't create file");
        }
    }
}

void handleClear()
{
    Serial.println("Handling settings clear");

    SPIFFS.remove(path_wifi_txt);
    SPIFFS.remove(path_lirc_txt);

    if (server.method() != HTTP_POST) 
    {
        server.send(405, "text/plain", "Method Not Allowed");
    } 
    else 
    {
        server.sendHeader("Location", String("/"), true);
        server.send ( 302, "text/plain", "");
    }
}

void handleReboot()
{
    Serial.println("Handling reboot");

    if (server.method() != HTTP_POST) 
    {
        server.send(405, "text/plain", "Method Not Allowed");
    } 
    else 
    {
        server.sendHeader("Location", String("/"), true);
        server.send ( 302, "text/plain", "");

        delay(1000);
        ESP.restart();
    }
}

void handleNotFound()
{
    server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}

bool ota = false; //false means soft_ap, true means ota

void setupConfig(bool forceAp)
{
    if(!forceAp && connect_wifi())
    {
        ArduinoOTA.onStart([]() {
            Serial.println("Start OTA");
        });
        ArduinoOTA.onEnd([]() {
            Serial.println("\nEnd OTA");
        });
        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
            Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
        });
        ArduinoOTA.onError([](ota_error_t error) {
            Serial.printf("Error[%u]: ", error);
            if (error == OTA_AUTH_ERROR)
                Serial.println("Auth Failed");
            else if (error == OTA_BEGIN_ERROR)
                Serial.println("Begin Failed");
            else if (error == OTA_CONNECT_ERROR)
                Serial.println("Connect Failed");
            else if (error == OTA_RECEIVE_ERROR)
                Serial.println("Receive Failed");
            else if (error == OTA_END_ERROR)
                Serial.println("End Failed");
        });
        ArduinoOTA.begin();
        ota = true;

        Serial.println("Ready for OTA");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    }
    else
    {        
        WiFi.mode(WIFI_AP_STA);
        
        Serial.print("Setting soft-AP ... ");
        boolean result = WiFi.softAP("ESPsoftAP_01");
        Serial.print("Server IP address: ");
        Serial.println(WiFi.softAPIP());
        Serial.print("Server MAC address: ");
        Serial.println(WiFi.softAPmacAddress());

        if(result == true)
        {
            Serial.println("Ready");
        }
        else
        {
            Serial.println("Failed!");
        }
    }

    server.on("/", handleRoot);
    server.on("/wifi/", HTTP_POST, handleWifiSettings);
    server.on("/lirc/", HTTP_POST, [](){ server.send(200); }, handleLircUpload);
    server.on("/clear/", HTTP_POST, handleClear);
    server.on("/reboot/", HTTP_POST, handleReboot);
    server.onNotFound(handleNotFound);

    server.begin();

    Serial.println("Webserver ready");
}

int i = 0;
void loopConfig()
{
    if(ota)
    {
        ArduinoOTA.handle();
    }
    else
    {
        if(++i%200000 == 0)
            Serial.printf("Stations connected = %d\n", WiFi.softAPgetStationNum());
    }

    server.handleClient();
}
