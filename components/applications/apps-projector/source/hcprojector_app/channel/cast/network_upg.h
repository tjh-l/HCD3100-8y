#ifndef __NETWORK_UPG_H__
#define __NETWORK_UPG_H__


typedef struct
{
    int status_code;//HTTP/1.1 '200' OK
    unsigned int content_length;//Content-Length
}http_res_header_t;

typedef enum
{
    NETWORK_UPG_IDEL = 0,
    NETWORK_UPG_DOWNLOAD,
    NETWORK_UPG_BURN,
} network_upg_status_e;


int network_upg_start(char *url);
int network_upg_download_config(char *url, char *buffer, int buffer_len);
void network_upg_set_user_abort(int flag);
int network_upg_get_status(void);

#endif

