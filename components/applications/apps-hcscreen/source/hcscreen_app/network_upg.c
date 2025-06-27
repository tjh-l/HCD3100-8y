
#include "app_config.h"

#ifdef NETWORK_SUPPORT

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <pthread.h>
#include <errno.h>


#include "com_api.h"
#include "network_api.h"
#include "cast_api.h"
#include "network_upg.h"

static int g_upgrade_status = 0;//0-idel, 1-download file, 2-burnflash.
static pthread_mutex_t g_upgrade_mutex = PTHREAD_MUTEX_INITIALIZER;
static char g_upgrade_url[1024] = {0};
static int g_user_abort = 0;

int network_upg_get_status()
{
    int status = 0;
    pthread_mutex_lock(&g_upgrade_mutex);
    status = g_upgrade_status;
    pthread_mutex_unlock(&g_upgrade_mutex);
    return status;
}

void network_upg_set_status(int status)
{
    pthread_mutex_lock(&g_upgrade_mutex);
    g_upgrade_status = status;
    pthread_mutex_unlock(&g_upgrade_mutex);
}

void network_upg_set_user_abort(int flag)
{
    g_user_abort = flag;
}

void network_upg_start_services(void)
{
    printf("[%s]  begin start services.\n", __func__);

#ifdef DLNA_SUPPORT
    hccast_dlna_service_start();
#ifdef DIAL_SUPPORT
    hccast_dial_service_start();
#endif
#endif
#ifdef AIRCAST_SUPPORT
    hccast_air_service_start();
#endif
#ifdef MIRACAST_SUPPORT
    hccast_mira_service_start();
#endif
}

void network_upg_stop_services(void)
{
    printf("[%s]  begin stop services.\n", __func__);
#ifdef DLNA_SUPPORT
    hccast_dlna_service_stop();
#ifdef DIAL_SUPPORT
    hccast_dial_service_stop();
#endif
#endif
#ifdef AIRCAST_SUPPORT
    hccast_air_service_stop();
#endif
#ifdef MIRACAST_SUPPORT
    hccast_mira_service_stop();
#endif
}

void network_upg_parse_url(const char *url, char *host, int *port, char *server_ip)
{
    int j = 0;
    int start = 0;
    *port = 80;
    char *patterns[] = {"http://", NULL};

    for (int i = 0; patterns[i]; i++)
        if (strncmp(url, patterns[i], strlen(patterns[i])) == 0)
            start = strlen(patterns[i]);

    //1.parse Host server name.
    for (int i = start; url[i] != '/' && url[i] != '\0'; i++, j++)
        host[j] = url[i];
    host[j] = '\0';

    //2.if has port num,parse it.
    char *pos = strstr(host, ":");
    if (pos)
    {
        sscanf(pos, ":%d", port);
        memcpy(server_ip,host,strlen(host));
        char* tmp = strstr(server_ip, ":");
        if(tmp)
        {
            *tmp = '\0';
        }
    }
    else
    {
        memcpy(server_ip,host,strlen(host));
    }
}

void network_upg_get_ip_addr(char *host_name, char *ip_addr)
{

    struct hostent *host = gethostbyname(host_name);
    if (!host)
    {
        ip_addr = NULL;
        return;
    }

    for (int i = 0; host->h_addr_list[i]; i++)
    {
        strcpy(ip_addr, inet_ntoa( * (struct in_addr*) host->h_addr_list[i]));
        break;
    }
}

int network_upg_send(int client_socket, char* buf, int len)
{
    int pos = 0;       
    int send_len = len;
    int ret = 0;

    if(buf == NULL)
    {
        return -1;
    }
    
    while(send_len)
    {
        ret = send(client_socket, buf, send_len, 0);
        if(ret < 0)
        {   
            if(errno == EINTR)
            {
                continue;
            }
            printf("%s send error.\n", __func__);
            return -1;
        }
        
        send_len -= ret;
        buf += ret;
    }

    return 0;
}

void network_upg_send_ap_msg(int msg_type, unsigned int msg_code)
{
    control_msg_t msg = {0};
    
    msg.msg_type = msg_type;
    msg.msg_code = msg_code;
    
    if(msg_type == MSG_TYPE_NET_UPGRADE)
    {
        network_upg_stop_services();
    }
    else if(msg_type == MSG_TYPE_UPG_STATUS)
    {
        if((msg_code == UPG_STATUS_SERVER_FAIL) ||(msg_code == UPG_STATUS_USER_STOP_DOWNLOAD))
        {
            network_upg_start_services();
        }    
    }
    
    api_control_send_msg(&msg);
}

int network_upg_parse_hearder(int client_socket, int *status_code, unsigned int *content_len)
{
    int mem_size = 4096;
    int length = 0;
    int len = 0;
    int ret = 0;
    char *buf = NULL;
    char *response = NULL;

    if ((status_code == NULL) || (content_len == NULL))
    {
        return -1;
    }

    buf = (char *) malloc(mem_size * sizeof(char));
    if(buf == NULL)
    {
        ret = -1;
        goto error_handler;
    }
    
    response = (char *) malloc(mem_size * sizeof(char));
    if(response == NULL)
    {
        ret = -1;
        goto error_handler;
    }
    memset(response, 0, mem_size * sizeof(char));

    while(1)
    {
        len = read(client_socket, buf, 1);
        if(len <= 0)
        {
            if(errno == EINTR)
            {
                continue;
            }
            
            ret = -1;
            printf("%s read fail\n", __func__);
            goto error_handler;
        }
        else
        {
            if (length + len > mem_size)
            {
                mem_size *= 2;
                char * temp = (char *) realloc(response, sizeof(char) * mem_size);
                if (temp == NULL)
                {
                    ret = -1;
                    printf("%s realloc fail\n", __func__);
                    goto error_handler;
                }
                response = temp;
				memset(response, 0, mem_size * sizeof(char));
            }

            buf[len] = '\0';
            strcat(response, buf);


            int flag = 0;
            for (int i = strlen(response) - 1; response[i] == '\n' || response[i] == '\r'; i--, flag++);

            if (flag == 4)
                break;

            length += len;
        }
    }


    char *pos = strstr(response, "HTTP/");
    if (pos)
    {
        sscanf(pos, "%*s %d", status_code);
    }    

    pos = strstr(response, "Content-Length:");
    if (pos)
    {
        sscanf(pos, "%*s %u", content_len);
    }    

    return 0;

error_handler:

    if(response)
    {
        free(response);
    } 

    if(buf)
    {
        free(buf);
    }    

    return ret;        
}

void network_upg_download_firmware(int client_socket,  unsigned int content_length)
{
    int ret = 0;
    long left_len = content_length;
    int downloap_len = 0;
    int progress = 0;
    int ui_progress = 0;

    char *buf = api_upgrade_buffer_alloc(content_length);
    if(buf == NULL)
    {
        printf("%s malloc upgrade buf fail\n", __func__);
        network_upg_set_status(NETWORK_UPG_IDEL);
        network_upg_send_ap_msg(MSG_TYPE_UPG_STATUS, UPG_STATUS_SERVER_FAIL); 
        return;
    }
    
    memset(buf, 0, content_length);

    while (left_len)
    {
        if(g_user_abort)
        {
            printf("user abort\n");
            network_upg_send_ap_msg(MSG_TYPE_UPG_STATUS, UPG_STATUS_USER_STOP_DOWNLOAD); 
            network_upg_set_status(NETWORK_UPG_IDEL);
            api_upgrade_buffer_free(buf);
            return;
        }
    
        ret = recv(client_socket, buf+downloap_len, left_len, 0);
        if(ret > 0)
        {
            left_len -= ret;
            downloap_len += ret;
            progress = downloap_len*100/content_length;
            if(progress != ui_progress)
            {
                ui_progress = progress;
                network_upg_send_ap_msg(MSG_TYPE_UPG_DOWNLOAD_PROGRESS, ui_progress); 
            }
        }
        else if(ret <= 0) //it mean server connect close.
        {
            network_upg_send_ap_msg(MSG_TYPE_UPG_STATUS, UPG_STATUS_SERVER_FAIL); 
            network_upg_set_status(NETWORK_UPG_IDEL);
            api_upgrade_buffer_free(buf);
            return;
        }

    }
    
    printf("%s successful len: %d\n", __func__, downloap_len);
    //callback to upper.
    network_upg_set_status(NETWORK_UPG_BURN);
    sys_upg_flash_burn(buf, content_length);
    network_upg_set_status(NETWORK_UPG_IDEL);

    if(buf)
    {
        api_upgrade_buffer_free(buf);
    }

}

static void *network_upg_thread(void *args)
{
    char* url = (char*)args;
    char host[128] = {0};
    char server_name[128] = {0};
    char ip_addr[16] = {0};
    int port = 80;
    char header[2048] = {0};
    int client_socket = -1;
    int retry = 2;
    int res = 0;

    network_upg_send_ap_msg(MSG_TYPE_NET_UPGRADE, 0);
    api_sleep_ms(500);

    network_upg_parse_url(url, host, &port, server_name);
    printf("%s Host: %s\n", __func__, host);
    printf("%s server_name: %s\n", __func__, server_name);
    printf("%s port: %d\n", __func__, port);

    network_upg_get_ip_addr(server_name, ip_addr);
    if (strlen(ip_addr) == 0)
    {
        printf("%s can not get remote server ip.\n", __func__);
        network_upg_set_status(NETWORK_UPG_IDEL);
        network_upg_send_ap_msg(MSG_TYPE_UPG_STATUS, UPG_STATUS_SERVER_FAIL);
        return NULL;
    }

    printf("%s remote ip: %s\n", __func__, ip_addr);

    sprintf(header, \
            "GET %s HTTP/1.1\r\n"\
            "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n"\
            "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537(KHTML, like Gecko) Chrome/47.0.2526Safari/537.36\r\n"\
            "Host: %s\r\n"\
            "Connection: keep-alive\r\n"\
            "\r\n"\
            ,url, host);

RETRY:
    client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_socket < 0)
    {
        printf("%s create socket fail\n", __func__);
        network_upg_set_status(NETWORK_UPG_IDEL);
        network_upg_send_ap_msg(MSG_TYPE_UPG_STATUS, UPG_STATUS_SERVER_FAIL);
        return NULL;
    }


    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip_addr);
    addr.sin_port = htons(port);

    res = connect(client_socket, (struct sockaddr *) &addr, sizeof(addr));
    if (res < 0)
    {
    	if(retry)
    	{		
            printf("%s connect retry\n", __func__);
            retry --;
            close(client_socket);
            client_socket = -1;
            usleep(1000*1000);
            goto RETRY;
        }

        perror("connect fail:");
        network_upg_set_status(NETWORK_UPG_IDEL);
        close(client_socket);
        network_upg_send_ap_msg(MSG_TYPE_UPG_STATUS, UPG_STATUS_SERVER_FAIL);
        return NULL;
    }
    printf("%s connect remote OK\n", __func__);

    //request HTTP GET.
    if(network_upg_send(client_socket, header, strlen(header)) < 0)
    {
        printf("%s send socket fail\n", __func__);
        network_upg_set_status(NETWORK_UPG_IDEL);
        close(client_socket);
        network_upg_send_ap_msg(MSG_TYPE_UPG_STATUS, UPG_STATUS_SERVER_FAIL); 
        return NULL;
    }

    http_res_header_t resp;
    if(network_upg_parse_hearder(client_socket, &resp.status_code, &resp.content_length) < 0)
    {
        printf("%s network_upg_parse_hearder fail\n", __func__);
        network_upg_set_status(NETWORK_UPG_IDEL);
        close(client_socket);
        network_upg_send_ap_msg(MSG_TYPE_UPG_STATUS, UPG_STATUS_SERVER_FAIL); 
        return NULL;
    }

    printf("\tHTTP status_code: %d\n", resp.status_code);
    printf("\tHTTP file size: %ld\n", resp.content_length);

    //look weather ok or not.
    if(resp.status_code == 200)
    {
        network_upg_download_firmware(client_socket,resp.content_length);
    }
    else
    {
        network_upg_send_ap_msg(MSG_TYPE_UPG_STATUS, UPG_STATUS_SERVER_FAIL); 
        network_upg_set_status(NETWORK_UPG_IDEL);
    }
    
    close(client_socket);
    return NULL;
}


void network_upg_start(char *url)
{
    pthread_t pid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x3000);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if(network_upg_get_status() != NETWORK_UPG_IDEL)
    {
        return;
    }

    memset(g_upgrade_url, 0, sizeof(g_upgrade_url));
    strcpy(g_upgrade_url, url);
    g_user_abort = 0;
    network_upg_set_status(NETWORK_UPG_DOWNLOAD);

    printf("beging to network_upg_start\n");
    if(pthread_create(&pid,&attr, network_upg_thread, (void*)g_upgrade_url) < 0)
    {
        printf("Create network_upg_thread error.\n");
        pthread_attr_destroy(&attr);
        return;
    }

    pthread_attr_destroy(&attr);
}


//download url data(no http header) to buffer.
//return the length of download url data. return <=0, error happen
int network_upg_download_config(char *url, char *buffer, int buffer_len)
{
    char host[128] = {0};
    char server_name[128] = {0};
    char ip_addr[16] = {0};
    int port = 80;
    char header[2048] = {0};
    int client_socket = -1;
    int ret;
    int downloap_len = 0;
    int retry = 2;

    network_upg_parse_url(url, host, &port, server_name);
    printf("Host: %s\n",  host);
    printf("server_name: %s\n", server_name);
    printf("port: %d\n", port);

    network_upg_get_ip_addr(server_name, ip_addr);
    if (strlen(ip_addr) == 0)
    {
        printf("can not get remote server ip.\n");
        return -1;
    }

    sprintf(header, \
            "GET %s HTTP/1.1\r\n"\
            "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n"\
            "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537(KHTML, like Gecko) Chrome/47.0.2526Safari/537.36\r\n"\
            "Host: %s\r\n"\
            "Connection: keep-alive\r\n"\
            "\r\n"\
            ,url, host);
            
RETRY:
    client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_socket < 0)
    {
        printf("create socket fail\n");
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip_addr);
    addr.sin_port = htons(port);

    int res = connect(client_socket, (struct sockaddr *) &addr, sizeof(addr));
    if (res < 0)
    {
        if(retry)
        {       
            printf("%s connect retry\n", __func__);
            retry --;
            close(client_socket);
            client_socket = -1;
            usleep(1000*1000);
            goto RETRY;
        }

        return -1;
    }

    //request HTTP GET.
    if(network_upg_send(client_socket, header, strlen(header)) < 0)
    {
        printf("send socket fail\n");
        close(client_socket);
        return -1;
    }

    http_res_header_t resp;
    if(network_upg_parse_hearder(client_socket, &resp.status_code, &resp.content_length) < 0)
    {
        printf("network_upg_parse_hearder fail\n");
        close(client_socket);
        return -1;
    }

    printf("\tHTTP status_code: %d\n", resp.status_code);
    printf("\tHTTP file size: %ld\n", resp.content_length);


    unsigned int content_length = resp.content_length;
    //look weather ok or not.
    downloap_len = 0;

    char *buf = buffer;
    if(resp.status_code == 200)
    {
        if(content_length)
        {
            unsigned int left_len = content_length ? (content_length > buffer_len ? buffer_len : content_length) : buffer_len;
            while (left_len)
            {
                ret = read(client_socket, buf+downloap_len, left_len);
                if(ret > 0)
                {
                    left_len -= ret;
                    downloap_len += ret;
                }
                else if(ret <= 0) //it mean server connect close.
                {
                    if(errno == EINTR)
                    {
                        continue;
                    }
                    close(client_socket);
                    return -1;
                }
            }
            printf("net_upgrade_download_config successful!\n");
        }
    }
    else
    {
        close(client_socket);
        return 0;
    }
    
    close(client_socket);

    return downloap_len;

}

#endif
