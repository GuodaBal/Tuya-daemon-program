#include <stdio.h>
#include <time.h>

#include "tuya_log.h"
#include "cJSON.h"

#include "action_handler.h"

extern void handle_action(char *data_string)
{
    cJSON *data = cJSON_Parse(data_string);
    if (data == NULL){
        return;
    }

    const cJSON *action = cJSON_GetObjectItem(data, "actionCode");

    if (action && strcmp(action->valuestring, "save_text") == 0) {

        cJSON *input = cJSON_GetObjectItem(data, "inputParams");
        const cJSON *text = cJSON_GetObjectItem(input, "text");

        if (cJSON_IsString(text) && text->valuestring != NULL)
            handle_save_text(text->valuestring);
        else
            TY_LOGE("Something went wrong with parsing\n");

    }
    

    cJSON_Delete(data);
}
extern void handle_save_text(const char *data_string)
{
    const char *file_path = "/tmp/tuya_action.log";
    FILE *fptr = fopen(file_path, "a");
    char log[1500];

    if(fptr == NULL){
        return;
    }

    time_t rawtime;
    const struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    char formatted_time[80] = "";
    strftime(formatted_time, 80, "%Y-%m-%d %H:%M:%S", timeinfo);

    snprintf(log, sizeof(log), "New message at %s: %s\n", formatted_time, data_string);
    fprintf(fptr, "%s", log);
    fclose(fptr);
}