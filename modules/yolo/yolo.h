#ifndef _YOLO_H_
#define _YOLO_H_

// these are loaded from card
struct network_config
{
    char SSID[32];
    char passphrase[64]; // I haven't tested max length keys, I don't know if DryOS
                         // treats this as null terminated or not
    uint32_t client_IP;
    uint32_t gateway_IP;
    uint32_t server_IP;
    uint16_t server_port;
};

#endif
