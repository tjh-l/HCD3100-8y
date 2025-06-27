#ifndef __HDCPD_SERVICE_H__
#define __HDCPD_SERVICE_H__

typedef void (*udhcp_lease_cb)(unsigned int yiaddr);
typedef int  (*udhcp_priv_cb)(unsigned int event, void* in, void* out);

#define BIT(nr) (1UL << (nr))
#define UDHCPC_HOSTNAME_LEN 32

// udhcp_conf_t option
typedef enum {
    UDHCPC_ABORT_IF_NO_LEASE         = BIT(0),
    UDHCPC_FOREGROUND                = BIT(1), // unsupported
    UDHCPC_QUIT_AFTER_LEASE          = BIT(2),
    UDHCPC_BACKGROUND_IF_NO_LEASE    = BIT(3), // unsupported
    UDHCPC_HOTSPOT_EN                = BIT(4),
} udhcp_option_e;

typedef enum {
    UDHCP_EVENT_NONE,
    UDHCP_EVENT_DHCPC   = 0x100,
    UDHCP_EVENT_DHCPD   = 0x200,
    UDHCP_EVENT_CUSTOM  = 0x500,
    UDHCP_EVENT_CUSTOM_HOTSPOT,
} udhcp_event_e;

typedef enum
{
    UDHCP_IF_NONE = 0,
    UDHCP_IF_WLAN0,
    UDHCP_IF_WLAN1,
    UDHCP_IF_P2P0,
} udhcp_ifname_e;

typedef struct
{
    char stat;
    char res[7];
    char ip[16];
    char mask[16];
    char gw[16];
    char dns[64];
    char ifname[32];
    char last_ip[16];
} hccast_udhcp_result_t;

typedef struct
{
    udhcp_lease_cb func;        // in
    udhcp_ifname_e ifname;      // in
    int pid;                    // out, dont set value.
    int run;                    // out, dont set value.
#ifdef __linux__
    int stop_pipe;              // out, dont set value.
#endif
    unsigned int option;        // in
    // unsigned long req_ip;    // Reserved, use only dhcpc, IP address to request (default: none)
    char ip_start_def[32];      // in, only support udhcpd
    char ip_end_def[32];        // in, only support udhcpd
    char ip_host_def[32];       // in, only support udhcpd
    char subnet_mask_def[32];   // in, only support udhcpd
    struct udhcp_priv
    {
        int state;
        unsigned long server_addr;
        unsigned long timeout;
        int packet_num;
        int fd;
        char hostname[UDHCPC_HOSTNAME_LEN];
        int listen_mode;
        udhcp_priv_cb func;
        unsigned long saved_ip;    // use only dhcpc, the successfully saved IP address from the last connection
    } priv;
} udhcp_conf_t;

#ifdef __cplusplus
extern "C" {
#endif

int udhcpd_start(udhcp_conf_t *conf);
int udhcpd_stop(udhcp_conf_t *conf);
int udhcpc_start(udhcp_conf_t *conf);
int udhcpc_stop(udhcp_conf_t *conf);
int udhcpc_set_hostname(char *hostname);


#ifdef __cplusplus
}
#endif

#endif
