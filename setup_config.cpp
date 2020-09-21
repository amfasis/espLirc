#include "setup_config.h"
#include "config.h"
#include <FS.h>
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>

#include "wifi.h"
#include "print.h"

ESP8266WebServer server(80);

String printIp(char source[4])
{
    return String((int)source[0]) + "." + String((int)source[1]) + "." + String((int)source[2]) + "." + String((int)source[3]);
}

String printMac(char source[6])
{
    return String((int)source[0], HEX) + ":" +
           String((int)source[1], HEX) + ":" +
           String((int)source[2], HEX) + ":" +
           String((int)source[3], HEX) + ":" +
           String((int)source[4], HEX) + ":" +
           String((int)source[5], HEX);
}

void parseIp(char *dest, const String& source)
{
    sscanf(source.c_str(), "%hhu.%hhu.%hhu.%hhu", &dest[0], &dest[1], &dest[2], &dest[3]);
}

String printSequence(int arr0, int arr1, int arr2, int arr3, int arr4)
{
    return String(arr0) + " " + 
           String(arr1) + " " + 
           String(arr2) + " " + 
           String(arr3) + " " + 
           String(arr4);
}

void parseMac(char *dest, const String& source)
{
    sscanf(source.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &dest[0], &dest[1], &dest[2], &dest[3], &dest[4], &dest[5]);
}

void handleRoot()
{
    PRINTLN("handling root");

    String body = "<html><body>";
    body += "<h1>Wifi settings</h1>";
    body += "<form method=\"post\" enctype=\"application/x-www-form-urlencoded\" action=\"/wifi/\">";
    body += "<input type=\"text\" name=\"ssid\" placeholder=\"ssid\" maxlength=\"15\" value=\"" + String(configWifi.ssid) + String("\"/><br/>");
    body += "<input type=\"text\" name=\"pass\" placeholder=\"pass\" maxlength=\"35\" value=\"" + String(configWifi.pass) + String("\"/><br/>");
    body += "<input type=\"text\" name=\"ip\" placeholder=\"ip\" value=\"" + printIp(configWifi.ip) + String("\"/> current: ") + WiFi.localIP().toString() + String("<br/>");
    body += "<input type=\"text\" name=\"nm\" placeholder=\"nm\" value=\"" + printIp(configWifi.netmask) + String("\"/><br/>");
    body += "<input type=\"text\" name=\"gw\" placeholder=\"gw\" value=\"" + printIp(configWifi.gateway) + String("\"/><br/>");
    body += "<input type=\"submit\" value=\"Opslaan\"/>";
    body += "</form>";

    body += "<h1>Lirc settings</h1>";
    body += "<form method=\"post\" enctype=\"multipart/form-data\" action=\"/lirc/\">";
    body += "<input type=\"file\" name=\"name\">";
    body += "<input class=\"button\" type=\"submit\" value=\"Upload\">";
    body += "</form>";

    body += "<h1>Remove config</h1>";
    body += "<form method=\"post\" enctype=\"application/x-www-form-urlencoded\" action=\"/clear/\">";
    body += "<input type=\"submit\" value=\"Clear\"/>";
    body += "</form>";
    
    body += "<h1>Reboot</h1>";
    body += "<form method=\"post\" enctype=\"application/x-www-form-urlencoded\" action=\"/reboot/\">";
    body += "<input type=\"submit\" value=\"Reboot\"/>";
    body += "<label><input type=\"checkbox\" name=\"config\"/>To config</label>";
    body += "</form>";
    body += "</body></html>";
    server.send(200, "text/html", body);
}

void handleWifiSettings()
{
    PRINTLN("handling wifi settings");
    if(server.hasArg("ssid"))
        server.arg("ssid").toCharArray(configWifi.ssid, sizeof(configWifi.ssid));
    if(server.hasArg("pass"))
        server.arg("pass").toCharArray(configWifi.pass, sizeof(configWifi.pass));
    
    if(server.hasArg("ip"))
        parseIp(configWifi.ip, server.arg("ip"));
    if(server.hasArg("gw"))
        parseIp(configWifi.gateway, server.arg("gw"));
    if(server.hasArg("nm"))
        parseIp(configWifi.netmask, server.arg("nm"));
    File configF2 = SPIFFS.open(path_wifi_txt, "w+");
    if(configF2)
    {
        unsigned char * data = reinterpret_cast<unsigned char*>(&configWifi);
        size_t bw = configF2.write(data, sizeof(configWifi));
        configF2.close();
        PRINTF("written length: %d\n", bw);
    }
    else
    {
        PRINTLN("could not open wifi.txt");
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
    PRINTLN("Handling lirc upload");

    // upload a new file to the SPIFFS
    HTTPUpload &upload = server.upload();
    if (upload.status == UPLOAD_FILE_START)
    {
        String filename = upload.filename;
        if (!filename.startsWith("/"))
            filename = "/" + filename;
        PRINTLN("handleFileUpload Name: ");
        PRINTLN(filename);
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
            PRINTLN("handleFileUpload Size: ");
            PRINTLN(upload.totalSize);
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
    PRINTLN("Handling settings clear");

    SPIFFS.remove(path_wifi_txt);
    SPIFFS.remove(path_lirc_txt);

    readConfig();

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
    PRINTLN("Handling reboot");

    if(server.hasArg("config"))
    {
        resetToStartConfig();
    }

    if (server.method() != HTTP_POST) 
    {
        server.send(405, "text/plain", "Method Not Allowed");
    } 
    else 
    {
        server.sendHeader("Location", String("/"), true);
        server.send ( 302, "text/plain", "");

        //Go to sleep
        PRINTLN("sleeping 3s");
        delay(3000);
        ESP.restart();
    }
}

void handleRssi()
{
    server.send(200, "text/html", "<html><body>RSSI: " + String(WiFi.RSSI()) + "</body></html");
}

void handleBlast()
{
    PRINTLN("Handling blast");

    String name = "header";
    for (int i = 0; i < 5;i++)
    {
        if (server.hasArg(name + i))
            configRemote.header[i] = server.arg(name + i).toInt();
    }
    name = "one";
    for (int i = 0; i < 5;i++)
    {
        if (server.hasArg(name + i))
            configRemote.one[i] = server.arg(name + i).toInt();
    }
    name = "zero";
    for (int i = 0; i < 5;i++)
    {
        if (server.hasArg(name + i))
            configRemote.zero[i] = server.arg(name + i).toInt();
    }
    name = "ptrail";
    for (int i = 0; i < 5;i++)
    {
        if (server.hasArg(name + i))
            configRemote.ptrail[i] = server.arg(name + i).toInt();
    }

    if(server.hasArg("gap"))
        configRemote.gap = server.arg("gap").toInt();

    if(server.hasArg("code"))
    {
        PRINTF("handling code %s\n", server.arg("code").c_str());
        String input = server.arg("code");
        int last = input.length() - 1;
        char buffer[32];
        memset(buffer, 0, sizeof(buffer));
        for (int i = 0; i < 8 && i <= last; i++)
        {
            buffer[32 - i * 4 - 1] = (input[last - i] == '1' ||
                                      input[last - i] == '3' ||
                                      input[last - i] == '5' ||
                                      input[last - i] == '7' ||
                                      input[last - i] == '9' ||
                                      input[last - i] == 'B' ||
                                      input[last - i] == 'D' ||
                                      input[last - i] == 'F') ? '1' : '0';
            buffer[32 - i * 4 - 2] = (input[last - i] == '2' ||
                                      input[last - i] == '3' ||
                                      input[last - i] == '6' ||
                                      input[last - i] == '7' ||
                                      input[last - i] == 'A' ||
                                      input[last - i] == 'B' ||
                                      input[last - i] == 'E' ||
                                      input[last - i] == 'F') ? '1' : '0';
            buffer[32 - i * 4 - 3] = (input[last - i] == '4' ||
                                      input[last - i] == '5' ||
                                      input[last - i] == '6' ||
                                      input[last - i] == '7' ||
                                      input[last - i] == 'C' ||
                                      input[last - i] == 'D' ||
                                      input[last - i] == 'E' ||
                                      input[last - i] == 'F') ? '1' : '0';
            buffer[32 - i * 4 - 4] = (input[last - i] == '8' ||
                                      input[last - i] == '9' ||
                                      input[last - i] == 'A' ||
                                      input[last - i] == 'B' ||
                                      input[last - i] == 'C' ||
                                      input[last - i] == 'D' ||
                                      input[last - i] == 'E' ||
                                      input[last - i] == 'F') ? '1' : '0';
        }

        blast(buffer);
    }

    String formStart = "<form action=\"/blast/\">";
    String formEnd = "<input type=\"submit\" value=\"Versturen\"/></form></br>";

    String body = "<html><body>";
    body += "<h1>Blast settings</h1>";

    body += formStart;
    body += "Header ";
    for (int i = 0; i < 5;i++)
    {
        body += " <input type =\"text\" name=\"header";
        body += i;
        body += "\" placeholder=\"header\" maxlength=\"15\" value=\"";
        body += configRemote.header[i];
        body += "\"/>";
    }
    body += formEnd;

    body += "One ";
    for (int i = 0; i < 5;i++)
    {
        body += " <input type =\"text\" name=\"one";
        body += i;
        body += "\" placeholder=\"one\" maxlength=\"15\" value=\"";
        body += configRemote.one[i];
        body += "\"/>";
    }
    body += formEnd;
    
    body += "Zero ";
    for (int i = 0; i < 5;i++)
    {
        body += " <input type =\"text\" name=\"zero";
        body += i;
        body += "\" placeholder=\"zero\" maxlength=\"15\" value=\"";
        body += configRemote.zero[i];
        body += "\"/>";
    }
    body += formEnd;
    
    body += "PTrail ";
    for (int i = 0; i < 5;i++)
    {
        body += " <input type =\"text\" name=\"ptrail";
        body += i;
        body += "\" placeholder=\"ptrail\" maxlength=\"15\" value=\"";
        body += configRemote.ptrail[i];
        body += "\"/>";
    }
    body += formEnd;

    body += formStart;
    body += "Gap <input type=\"text\" name=\"gap\" placeholder=\"gap\" maxlength=\"15\" value=\"" + String(configRemote.gap) + String("\"/>");
    body += formEnd;

    body += "<br>Send:" + formStart;
    body += "0x<input type=\"text\" name=\"code\" placeholder=\"code\" maxlength=\"15\" value=\"" + server.arg("code") + String("\"/><br/>");
    body += formEnd;

    body += "</body></html>";

    server.send(200, "text/html", body);
}

void handleNotFound()
{
//    server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
    server.sendHeader("Location", String("/"), true);
    server.send ( 302, "text/plain", "");
}

bool ota = false; //false means soft_ap, true means ota

void setupConfig(bool forceAp)
{
    if(!forceAp && connect_wifi())
    {
        ArduinoOTA.onStart([]() {
            PRINTLN("Start OTA");
        });
        ArduinoOTA.onEnd([]() {
            PRINTLN("\nEnd OTA");
        });
        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
            PRINTF("Progress: %u%%\r", (progress / (total / 100)));
        });
        ArduinoOTA.onError([](ota_error_t error) {
            PRINTF("Error[%u]: ", error);
            if (error == OTA_AUTH_ERROR)
                PRINTLN("Auth Failed");
            else if (error == OTA_BEGIN_ERROR)
                PRINTLN("Begin Failed");
            else if (error == OTA_CONNECT_ERROR)
                PRINTLN("Connect Failed");
            else if (error == OTA_RECEIVE_ERROR)
                PRINTLN("Receive Failed");
            else if (error == OTA_END_ERROR)
                PRINTLN("End Failed");
        });
        ArduinoOTA.begin();
        ota = true;

        PRINTLN("Ready for OTA");
        PRINTLN("IP address: ");
        PRINTLN(WiFi.localIP());
    }
    else
    {        
        WiFi.mode(WIFI_AP_STA);
        
        PRINTLN("Setting soft-AP ...");
        boolean result = WiFi.softAP("ESPsoftAP_01");
        PRINTF("Server IP address: ");
        PRINTLN(WiFi.softAPIP());
        PRINTF("Server MAC address: ");
        PRINTLN(WiFi.softAPmacAddress());

        if(result == true)
        {
            PRINTLN("Ready");
        }
        else
        {
            PRINTLN("Failed!");
        }
    }

    server.on("/", handleRoot);
    server.on("/wifi/", HTTP_POST, handleWifiSettings);
    server.on("/lirc/", HTTP_POST, [](){ server.send(200); }, handleLircUpload);
    server.on("/clear/", HTTP_POST, handleClear);
    server.on("/reboot/", HTTP_POST, handleReboot);
    server.on("/rssi/", handleRssi);
    server.on("/blast/", handleBlast);
    server.onNotFound(handleNotFound);

    server.begin();

    PRINTLN("Webserver ready");
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
        if(WiFi.getMode() != WIFI_AP_STA)
        {
            if(++i%100000 == 0)
            {
                PRINTF("RSSI: %d\n", WiFi.RSSI());
            }
        }
        else
        {
            if(++i%400000 == 0)
                PRINTF("Stations connected = %d\n", WiFi.softAPgetStationNum());
        }
    }

    server.handleClient();
}
