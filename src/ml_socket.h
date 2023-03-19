#ifndef _ML_SOCKET_H_
#define _ML_SOCKET_H_

// DryOS on modern cams has a socket library, with standard bind(), connect()
// etc functions

// I didn't want to call it socket.h, because that conflicts with a system library,
// and ML code has a bad habit of using <blah.h> for non-system libs

static const int16_t SOCK_FAMILY_IPv4 = 0x100; // network order 1
static const int16_t SOCK_FAMILY_IPv6 = 0x200;
static const int16_t SOCK_FAMILY_ETH = 0x300;

struct sockaddr_in {
    int16_t sin_family;
    uint16_t sin_port;
    uint32_t sin_addr;
    char sin_zero[8]; // haven't seen DryOS use this but
                      // we probably need it for compliance
};

int socket_convertfd(int socket);
int socket_create(int domain, int type, int protocol);
int socket_bind(int socket, struct sockaddr_in *addr, int addr_len);
int socket_connect(int socket, struct sockaddr_in *addr, int addr_len);
int socket_listen(int socket, int backlog);
int socket_accept(int socket, void *addr, int addr_len);
int socket_recv(int socket, void *buf, int len, int flags);
int socket_send(int socket, void *buf, int len, int flags);
void socket_setsockopt(int socket, int level, int option_name, const void *option_value, int option_len);
int socket_shutdown(int socket, int flag);
int socket_close_caller(int converted_socket);
int socket_select_caller(int converted_socket, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);

// not strictly socket related, but currently the only thing using sockets is wifi,
// so it can live in here for now.
static const uint32_t WLAN_CIPHER_MODE_NONE = 0;
static const uint32_t WLAN_CIPHER_MODE_WEP = 1;
static const uint32_t WLAN_CIPHER_MODE_AES = 4;

static const uint32_t WLAN_AUTH_MODE_OPEN = 0;
static const uint32_t WLAN_AUTH_MODE_SHARED = 1;
static const uint32_t WLAN_AUTH_MODE_WPA2PSK = 5;
static const uint32_t WLAN_AUTH_MODE_BOTH = 6;

static const uint32_t WLAN_MODE_ADHOC_WIFI = 1;
static const uint32_t WLAN_MODE_INFRA = 2;
static const uint32_t WLAN_MODE_ADHOC_G = 3;

struct wlan_settings
{
    uint32_t unk_01;
    uint32_t mode; // adhoc-wifi, adhoc-g, infra
    uint32_t mode_something; // adhoc-wifi = 1, adhoc-g = 2
    char SSID[0x24];
    uint32_t channel;
    uint32_t auth_mode; // open, shared, wpa2psk, both
    uint32_t cipher_mode; // WEP or AES
    uint32_t unk_02;
    uint32_t unk_03; // set to 6 when using INFRA or BOTH
    char key[0x3f];
    uint8_t unk_04[0x79];
} __attribute__((packed));
SIZE_CHECK_STRUCT(wlan_settings, 0xfc);

int wlan_connect(struct wlan_settings *settings);

#endif
