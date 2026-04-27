extern int tuya_connect_device(tuya_mqtt_context_t* client, char* device_id, char* device_secret);
extern void tuya_disconnect_device(tuya_mqtt_context_t* client);
extern void send_device_info(tuya_mqtt_context_t* context, char* device_id);
extern void main_tuya_loop(tuya_mqtt_context_t* client, char* device_id, int secs_between_sending_info);