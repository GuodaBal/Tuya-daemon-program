#include <stdlib.h>
#include "tuya_cacert.h"
#include "tuya_log.h"
#include "tuya_error_code.h"
#include "system_interface.h"
#include "mqtt_client_interface.h"
#include "tuyalink_core.h"

#include "device_info.h"
#include "action_handler.h"
#include "tuya_actions.h"

tuya_mqtt_context_t* send_info_context = NULL;
time_t last_send_time = 0;

void on_connected(tuya_mqtt_context_t* context, void* user_data);
void on_messages(tuya_mqtt_context_t* context, void* user_data, const tuyalink_message_t* msg);

extern void main_tuya_loop(tuya_mqtt_context_t* client, char* device_id, int secs_between_sending_info)
{
    tuya_mqtt_loop(client);
    time_t secs_since_last_loop = time(NULL) - last_send_time;
    if(secs_since_last_loop >= secs_between_sending_info){
        send_device_info(client, device_id);
        last_send_time = time(NULL);
    }
}

extern int tuya_connect_device(tuya_mqtt_context_t* client, char* device_id, char* device_secret)
{
    int ret = OPRT_OK;

    ret = tuya_mqtt_init(client, &(const tuya_mqtt_config_t) {
        .host = "m1.tuyacn.com",
        .port = 8883,
        .cacert = tuya_cacert_pem,
        .cacert_len = sizeof(tuya_cacert_pem),
        .device_id = device_id,
        .device_secret = device_secret,
        .keepalive = 100,
        .timeout_ms = 2000,
        .on_connected = on_connected,
        //.on_disconnect = on_disconnect,
        .on_messages = on_messages
    });

    if (ret != OPRT_OK) {
        TY_LOGE("mqtt init failed: %d", ret);
        return EXIT_FAILURE;
    }
    
    ret = tuya_mqtt_connect(client);

    if (ret != OPRT_OK) {
        TY_LOGE("mqtt connect failed: %d", ret);
        tuya_mqtt_disconnect(client);
        return EXIT_FAILURE;
    }

    return ret;
}

extern void tuya_disconnect_device(tuya_mqtt_context_t* client)
{
    tuya_mqtt_disconnect(client);
    tuya_mqtt_deinit(client);
}

extern void send_device_info(tuya_mqtt_context_t* context, char* device_id)
{
    unsigned long total_ram = get_total_ram();
    unsigned long free_ram = get_free_ram();
    int cpu_usage = get_cpu_usage();
    int uptime = get_uptime();
    char* interfaces = get_network_interfaces();

    cJSON *json = cJSON_CreateObject();

    cJSON_AddNumberToObject(json, "total_ram", total_ram);
    cJSON_AddNumberToObject(json, "free_ram", free_ram);
    cJSON_AddNumberToObject(json, "cpu_usage", cpu_usage);
    cJSON_AddNumberToObject(json, "system_uptime", uptime);
    cJSON_AddStringToObject(json, "network_interfaces", interfaces);

    char *payload = cJSON_PrintUnformatted(json);
    if(payload == NULL)
        return;
    
    tuyalink_thing_property_report(context, device_id, payload);
    free(interfaces);
    cJSON_Delete(json);
    cJSON_free(payload);
}

void on_connected(tuya_mqtt_context_t* context, void* user_data)
{
    send_info_context = context;
}

void on_messages(tuya_mqtt_context_t* context, void* user_data, const tuyalink_message_t* msg)
{
    TY_LOGI("on message id:%s, type:%d, code:%d", msg->msgid, msg->type, msg->code);
    switch (msg->type) {
        case THING_TYPE_ACTION_EXECUTE:
            TY_LOGI("executing action:%s\r\n", msg->data_string);
            handle_action(msg->data_string);
            break;
        default:
            break;
    }
    printf("\r\n");
}
