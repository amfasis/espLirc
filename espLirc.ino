/* 

*/
#include <FS.h>
#include "wifi.h"
#include "setup_config.h"
#include "config.h"
#include "print.h"

// #include <GDBStub.h>

//how many clients should be able to telnet to this ESP8266
#define MAX_SRV_CLIENTS 1

// --- RTC adresses ---
uint32_t rtc_mode = 0x31;
uint32_t mode_start_config = 0x12345678;
uint32_t mode_end_config   = 0x23456789;
uint32_t mode_start_normal = 0x3456789A;
uint32_t mode_end_normal   = 0x00000000;

bool configMode = false;

WiFiServer lirc_server(7059);
WiFiClient serverClients[MAX_SRV_CLIENTS];


char sbuffer[1024];

size_t selectedKeyPos = 0;
char blastType = 0; //0 = nothing, 1 = once, 2 = repeatedly

int IRpin = D1;

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(D1, OUTPUT);
    digitalWrite(D1, LOW);
    digitalWrite(LED_BUILTIN, LOW);

    Serial.begin(115200);
    SPIFFS.begin();

    // gdbstub_init();

    readConfig();

    uint32_t mode_magic;
    ESP.rtcUserMemoryRead(rtc_mode, &mode_magic, sizeof(mode_magic));

    if (mode_magic == mode_start_config ||
        mode_magic == mode_start_normal ||
        !SPIFFS.exists(path_wifi_txt) || 
        !SPIFFS.exists(path_lirc_txt) ||
        true)
    {
        PRINTLN("** Config boot **");
        ESP.rtcUserMemoryWrite(rtc_mode, &mode_start_config, sizeof(mode_magic));

        if (mode_magic == mode_start_normal)
        {
            PRINTLN("double reset detected");
        }
        else if (mode_magic == mode_start_config)
        {
            PRINTLN("triple reset detected");
        }

        if (!SPIFFS.exists(path_wifi_txt))
        {
            PRINTLN("no wifi");
        }
        if (!SPIFFS.exists(path_lirc_txt))
        {
            PRINTLN("no lirc");
        }

        setupConfig(mode_magic == mode_start_config);
        configMode = true;

        ESP.rtcUserMemoryWrite(rtc_mode, &mode_end_config, sizeof(mode_magic));
    }
    else
    {
        PRINTLN("** Normal boot **");
        ESP.rtcUserMemoryWrite(rtc_mode, &mode_start_normal, sizeof(mode_magic));

        if(!connect_wifi() && mode_magic == mode_end_config) //could not cfg to wifi after config boot, reset to config
        {
            PRINTLN("Wifi after config failed, restarting");
            ESP.restart();
        }

        //start UART and the server
        lirc_server.begin();
        lirc_server.setNoDelay(true);

        PRINTF("Ready!");
        PRINTF("IP: ");
        PRINTLN(WiFi.localIP());

        digitalWrite(LED_BUILTIN, HIGH);

        //analogWriteFreq(configRemote.frequency);

        ESP.rtcUserMemoryWrite(rtc_mode, &mode_end_normal, sizeof(mode_magic));
    }

    strcpy(sbuffer, "BEGIN\n");

    // gdbstub_init();
}

void loopNormal()
{
    uint8_t i;
    //check if there are any new clients
    if (lirc_server.hasClient())
    {
        for (i = 0; i < MAX_SRV_CLIENTS; ++i)
        {
            //find free/disconnected spot
            if (!serverClients[i] || !serverClients[i].connected())
            {
                if (serverClients[i])
                    serverClients[i].stop();
                serverClients[i] = lirc_server.available();
                PRINTF("New client: ");
                PRINTLN(serverClients[i].remoteIP());
                continue;
            }
        }
        //no free/disconnected spot so reject
        WiFiClient serverClient = lirc_server.available();
        serverClient.stop();
    }

    digitalWrite(LED_BUILTIN, HIGH);
    //check if the client writes the magic string to put device into config mode
    for (i = 0; i < MAX_SRV_CLIENTS;++i)
    {
        if(serverClients[i]) //live connection
            digitalWrite(LED_BUILTIN, LOW);

        if(serverClients[i].available())
        {
            char input[64];
            int read = serverClients[i].readBytesUntil('\n', input, 63);
            input[read] = 0;
            if (strcmp(input, "#OTA") == 0)
            {
                ESP.rtcUserMemoryWrite(rtc_mode, &mode_start_normal, sizeof(mode_start_normal));
                ESP.restart();
            }

            PRINTF(">");
            PRINTLN(input);

            strcpy(sbuffer + 6, input);
            size_t pos = 6 + strlen(input);
            sbuffer[pos++] = '\n';

            char *separator = strchr(input, ' ');
            if(separator != 0) //space was found
            {
                *separator = 0; //split the input
                char *directive = input;
                char *remote = ++separator;

                separator = strchr(separator, ' ');
                if(separator == 0) //LIST <remote>
                {
                    if(strcmp(directive, "LIST") == 0)
                    {
                        if(strcmp(remote, configRemote.name) == 0)
                        {
                            PRINTLN("Going to send keys");
                            pos += sprintf(sbuffer + pos, "SUCCESS\nDATA\n%d\n", configRemote.nb_codes);
                            size_t kpos = 0;
                            for (char i = 0; i < configRemote.nb_codes; ++i)
                            {
                                pos += sprintf(sbuffer + pos, "%d %s\n", i, configKeys + kpos);
                                kpos += 16 + configRemote.nb_bits;
                                
                                if(kpos + configRemote.nb_bits >= sizeof(configKeys))
                                    break;
                            };
                            strcpy(sbuffer + pos, "END\n");
                        }
                        else
                        {
                            strcpy(sbuffer + pos, "ERROR\nDATA\n1\nUnknown remote\nEND\n");
                        }
                    }
                    else
                    {
                        strcpy(sbuffer + pos, "ERROR\nDATA\n1\nUnknown directive\nEND\n");
                    }                    
                }
                else //LIST <remote> <key>
                {
                    *separator = 0; //split the input
                    char *key = ++separator;

                    if(strcmp(remote, configRemote.name) == 0)
                    {
                        if(strcmp(directive, "SEND_ONCE") == 0)
                        {
                            PRINTLN("send once");
                            size_t kpos = 0;
                            blastType = 0;
                            for (char i = 0; i < configRemote.nb_codes; ++i)
                            {
                                if(strcmp(configKeys + kpos, key) == 0)
                                {
                                    selectedKeyPos = kpos;
                                    blastType = 1;
                                    break;
                                }
                                kpos += 16 + configRemote.nb_bits;
                                
                                if(kpos + configRemote.nb_bits >= sizeof(configKeys))
                                    break;
                            }

                            if(blastType == 1)
                                strcpy(sbuffer + pos, "SUCCESS\nEND\n");
                            else
                                strcpy(sbuffer + pos, "ERROR\nDATA\n1\nNot found\nEND\n");
                        }
                        else if(strcmp(directive, "SEND_START") == 0)
                        {
                            PRINTLN("send start");
                            size_t kpos = 0;
                            blastType = 0;
                            for (char i = 0; i < configRemote.nb_codes; ++i)
                            {
                                if(strcmp(configKeys + kpos, key) == 0)
                                {
                                    selectedKeyPos = kpos;
                                    blastType = 2;
                                    break;
                                }
                                kpos += 16 + configRemote.nb_bits;
                                
                                if(kpos + configRemote.nb_bits >= sizeof(configKeys))
                                    break;
                            }

                            if(blastType == 2)
                                strcpy(sbuffer + pos, "SUCCESS\nEND\n");
                            else
                                strcpy(sbuffer + pos, "ERROR\nDATA\n1\nNot found\nEND\n");
                        }   
                        else if(strcmp(directive, "SEND_STOP") == 0)
                        {
                            PRINTLN("send stop");
                            blastType = 0;
                            strcpy(sbuffer + pos, "SUCCESS\nEND\n");
                        } 
                        else if(strcmp(directive, "LIST") == 0)
                        {
                            PRINTLN("send key info");
                            strcpy(sbuffer + pos, "ERROR\nDATA\n1\nNot implemented\nEND\n");
                        }
                    }
                    else
                    {
                        strcpy(sbuffer + pos, "ERROR\nDATA\n1\nUnknown remote\nEND\n");
                    }
                }
            }
            else
            {
                /* directive only? */
                if(strcmp(input, "LIST") == 0)
                {
                    sprintf(sbuffer + pos, "SUCCESS\nDATA\n1\n%s\nEND\n", configRemote.name);
                }
                else
                {
                    strcpy(sbuffer + pos, "ERROR\nDATA\n1\nUnknown directive\nEND\n");
                }                
            }

            serverClients[i].write(sbuffer, strlen(sbuffer));
            PRINTF("%s", sbuffer);
        }
    }
    
    if(blastType > 0 && selectedKeyPos < configKeysSize)
    {
        PRINTF("Blasting %s\n", configKeys + selectedKeyPos);

        blast(configKeys + selectedKeyPos + 16);

        if (blastType == 1)
            blastType = 0;
        else
        {
            _delayMicroseconds(configRemote.gap);
        }
    }

    if (WiFi.status() != WL_CONNECTED)
    {
        ESP.restart();
    }
}

void blast(const char *bits)
{
    for (char i = 0; i < 5;++i)
    {
        mark_or_space(i, configRemote.header[i]);
    }

    for (char b = 0; b < configRemote.nb_bits;++b)
    {
        if(bits[b] == '1')
        {
            Serial.printf("1");
            for (char i = 0; i < 5; ++i)
            {
                mark_or_space(i, configRemote.one[i]);
            }
        }
        else if(bits[b] == '0')
        {
            Serial.printf("0");
            for (char i = 0; i < 5;++i)
            {
                mark_or_space(i, configRemote.zero[i]);
            }
        }
    }

    for (char i = 0; i < 5;++i)
    {
        mark_or_space(i, configRemote.ptrail[i]);
    }

}

void mark_or_space(char i, uint16_t time)
{
    if(i % 2 == 0)
        mark(time);
    else
        space(time);
}

const uint16_t kMaxAccurateUsecDelay = 16383;
void _delayMicroseconds(uint32_t usec) 
{
    for (; usec > kMaxAccurateUsecDelay; usec -= kMaxAccurateUsecDelay)
    {
        delayMicroseconds(kMaxAccurateUsecDelay);
    }
    delayMicroseconds(static_cast<uint16_t>(usec));
}

void mark(int time)
{
    // Sends an IR mark for the specified number of microseconds.
    // The mark output is modulated at the PWM frequency.
    //TIMER_ENABLE_PWM; // Enable pin 3 PWM output
    //if (time > 0) delayMicroseconds(time);
    long beginning = micros();
    while (micros() - beginning < time)
    {
        digitalWrite(IRpin, HIGH);
        delayMicroseconds(13);
        digitalWrite(IRpin, LOW);
        delayMicroseconds(13); //38 kHz -> T = 26.31 microsec (periodic time), half of it is 13
    }
}

void space(int time)
{
    // Sends an IR space for the specified number of microseconds.
    // A space is no output, so the PWM output is disabled.
    analogWrite(IRpin, 0);
    if (time > 0)
        _delayMicroseconds(time);
}

void loop()
{
    if (configMode)
    {
        loopConfig();
    }
    else
    {
        loopNormal();
    }
}

void resetToStartConfig()
{
    ESP.rtcUserMemoryWrite(rtc_mode, &mode_start_normal, sizeof(mode_start_normal));
}

void resetToEndConfig()
{
    ESP.rtcUserMemoryWrite(rtc_mode, &mode_end_config, sizeof(mode_end_config));
}
