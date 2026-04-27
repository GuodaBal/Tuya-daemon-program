#include <sys/sysinfo.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <net/if.h>

#include "cJSON.h"
#include "tuyalink_core.h"
#include "tuya_log.h"
#include "tuya_error_code.h"
#include "interface_info.h"

struct net_interface{
    char name[50];
    char ip[50];
    char mask[50];
    int received_data;
    int transmitted_data;
};

static int get_rx_tx_data_bytes(const char* ifa_name, const int return_tx_flag);
static char* get_interface_ip(struct ifaddrs *interface);
static char* get_interface_netmask(struct ifaddrs *interface);
static int get_received_data_bytes(const char* ifa_name);
static int get_transmitted_data_bytes(const char* ifa_name);

//Returns number of interfaces found
int get_interfaces(struct net_interface *interfaces)
{
    struct ifaddrs *networks = NULL;
    struct ifaddrs *temp = NULL;

    if (getifaddrs(&networks) != 0) {
        return 0;
    }

    int i = 0;
    for (temp = networks; temp != NULL; temp = temp->ifa_next) {
        if (!temp->ifa_addr)
            continue;

        if (temp->ifa_flags & IFF_LOOPBACK)
            continue;

        char *ip = get_interface_ip(temp);
        char *netmask = get_interface_netmask(temp);

        if (ip == NULL || netmask == NULL){
            continue;
        }

        int rx = get_received_data_bytes(temp->ifa_name);
        int tx = get_transmitted_data_bytes(temp->ifa_name);

        strncpy(interfaces[i].name, temp->ifa_name, sizeof(interfaces[i].name) - 1);
        strncpy(interfaces[i].ip, ip, sizeof(interfaces[i].ip) - 1);
        strncpy(interfaces[i].mask, netmask, sizeof(interfaces[i].mask) - 1);
        interfaces[i].received_data = rx;
        interfaces[i].transmitted_data = tx;

        free(ip);
        free(netmask);

        i++;
    }

    freeifaddrs(networks);
    return i;
}

cJSON* convert_interfaces_to_json(struct net_interface *interfaces, int icount)
{
    if(icount <= 0)
        return NULL;

    cJSON *json = cJSON_CreateObject();
    if (!json)
        return NULL;

    cJSON *arr = cJSON_AddArrayToObject(json, "network_interface");
    if (!arr) {
        cJSON_Delete(json);
        return NULL;
    }

    int i = 0;

    while(i < icount){
        cJSON *interface = cJSON_CreateObject();
        if (!interface)
            return NULL;
        cJSON_AddStringToObject(interface, "interface_name", interfaces[i].name);
        cJSON_AddStringToObject(interface, "ip_address", interfaces[i].ip);
        cJSON_AddStringToObject(interface, "netmask", interfaces[i].mask);
        cJSON_AddNumberToObject(interface, "received_data_amount", interfaces[i].received_data);
        cJSON_AddNumberToObject(interface, "transmitted_data_amount", interfaces[i].transmitted_data);

        cJSON_AddItemToArray(arr, interface);
        i++;
    }

    return json;
}

extern char* get_network_interfaces()
{
    struct net_interface *interfaces = malloc(10 * sizeof(struct net_interface));
    int count = get_interfaces(interfaces);
    cJSON *json = convert_interfaces_to_json(interfaces, count);
    free(interfaces);
    
    char *whole_string = cJSON_PrintUnformatted(json);
    char *sized_string = malloc(1024 / 2 + 1);
    if (whole_string == NULL || sized_string == NULL)
        return "\0";
    sized_string[0] = '\0';
    strncat(sized_string, whole_string, 1024 / 2);
    cJSON_Delete(json);
    cJSON_free(whole_string);
    return sized_string;
}

static char* get_interface_ip(struct ifaddrs *interface)
{
    char *ip = malloc(INET6_ADDRSTRLEN);
    if (ip == NULL)
        return NULL;
    ip[0] = '\0';
    if (interface->ifa_addr) {
        if (interface->ifa_addr->sa_family == AF_INET) {
            inet_ntop(AF_INET,
                    &((struct sockaddr_in *)interface->ifa_addr)->sin_addr,
                    ip, INET6_ADDRSTRLEN);
        } else if (interface->ifa_addr->sa_family == AF_INET6) {
            inet_ntop(AF_INET6,
                    &((struct sockaddr_in6 *)interface->ifa_addr)->sin6_addr,
                    ip, INET6_ADDRSTRLEN);
        }
    }
    if (strlen(ip) <= 1){
        strcpy(ip, "-");
    }
    return ip;
}


static char* get_interface_netmask(struct ifaddrs *interface)
{
    char *netmask = malloc(INET6_ADDRSTRLEN);
    if (netmask == NULL)
        return NULL;
    netmask[0] = '\0';
    if (interface->ifa_netmask) {
        if (interface->ifa_netmask->sa_family == AF_INET) {
            inet_ntop(AF_INET,
                    &((struct sockaddr_in *)interface->ifa_netmask)->sin_addr,
                    netmask, INET6_ADDRSTRLEN);
        } else if (interface->ifa_netmask->sa_family == AF_INET6) {
            inet_ntop(AF_INET6,
                    &((struct sockaddr_in6 *)interface->ifa_netmask)->sin6_addr,
                    netmask, INET6_ADDRSTRLEN);
        }
    }
    if (strlen(netmask) <= 1){
        strcpy(netmask, "-");
    }
    return netmask;
}

static int get_received_data_bytes(const char* ifa_name)
{
    int bytes = get_rx_tx_data_bytes(ifa_name, 0);
    return bytes;
}

static int get_transmitted_data_bytes(const char* ifa_name)
{
    int bytes = get_rx_tx_data_bytes(ifa_name, 1);
    return bytes;
}


static int get_rx_tx_data_bytes(const char* ifa_name, const int return_tx_flag)
{
    long data_bytes;
    int data_MiB;
    char buffer[1024] = "";
    char file_name[50] = "";
    if(return_tx_flag == 1)
        sprintf(file_name, "/sys/class/net/%s/statistics/tx_bytes", ifa_name);
    else
        sprintf(file_name, "/sys/class/net/%s/statistics/rx_bytes", ifa_name);
    FILE *file = fopen(file_name, "r");
    if(file == NULL) {
        return 0; 
    }
    fread(buffer, 1024, 1, file);
    fclose(file);
    if (sscanf(buffer, "%li", &data_bytes) == EOF)
        return 0;
    data_MiB = (int)(data_bytes/1048576);
    return data_MiB;
}