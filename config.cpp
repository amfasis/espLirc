#include "config.h"
#include <string.h>
#include <FS.h>
#include "print.h"

WifiConfig configWifi;
LircRemoteConfig configRemote;
char configKeys[2048];
size_t configKeysSize = 0;

void readConfig()
{
    File configW = SPIFFS.open(path_wifi_txt, "r");
    if(configW)
    {
        configW.readBytes((char*)&configWifi, sizeof(configWifi));
        configW.close();
    }
    else
    {
        memset(&configWifi, 0, sizeof(configWifi));
    }

    File configL = SPIFFS.open(path_lirc_txt, "r");
    if(configL)
    {
        configL.readBytes((char*)&configRemote, sizeof(configRemote));
        configKeysSize = configL.readBytes(configKeys, sizeof(configKeys));
        configL.close();
    }
    else
    {
        memset(&configRemote, 0, sizeof(configRemote));
        memset(configKeys, 0, sizeof(configKeys));
        configKeysSize = 0;
    }

    // print current config
    PRINTLN("Wifi config:");
    PRINTF("ssid: %s\n", configWifi.ssid);
    PRINTF("pass: %s\n", configWifi.pass);
    PRINTF("IP: %d.%d.%d.%d\n", (int)configWifi.ip[0], (int)configWifi.ip[1], (int)configWifi.ip[2], (int)configWifi.ip[3]);
    PRINTF("NM: %d.%d.%d.%d\n", (int)configWifi.netmask[0], (int)configWifi.netmask[1], (int)configWifi.netmask[2], (int)configWifi.netmask[3]);
    PRINTF("GW: %d.%d.%d.%d\n", (int)configWifi.gateway[0], (int)configWifi.gateway[1], (int)configWifi.gateway[2], (int)configWifi.gateway[3]);


    PRINTF("sizeof: %d\n", sizeof(configRemote));
    PRINTF("Number of bits: %d\n", configRemote.nb_bits);
    PRINTF("frequency: %d\n", configRemote.frequency);
    for(int i=0;i<5;++i) PRINTF("header: %d\n", configRemote.header[i]);
    for(int i=0;i<5;++i) PRINTF("one: %d\n", configRemote.one[i]);
    for(int i=0;i<5;++i) PRINTF("zero: %d\n", configRemote.zero[i]);
    for(int i=0;i<5;++i) PRINTF("ptrail: %d\n", configRemote.ptrail[i]);
    PRINTF("gap length: %d\n", configRemote.gap);
    PRINTF("Number of codes: %d\n", (unsigned short)configRemote.nb_codes);
    PRINTF("Key config size: %d\n", configKeysSize);
}