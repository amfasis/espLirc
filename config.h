#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

// --- config structs ---
struct WifiConfig
{
    char ssid[16];
    char pass[36];
    char ip[4];
    char gateway[4];
    char netmask[4];
};

struct __attribute__ ((packed)) LircRemoteConfig
{
    char name[16];
    uint8_t nb_bits;
    uint32_t frequency;
    uint16_t header[5];
    uint16_t one[5];
    uint16_t zero[5];
    uint16_t ptrail[5];
    uint32_t gap;
    uint8_t nb_codes;
};


#define path_wifi_txt "/wifi.txt"
#define path_lirc_txt "/lirc.txt"

#endif /* CONFIG_H */
