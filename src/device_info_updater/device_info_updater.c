#include <stdio.h>
#include <stdlib.h>
#include <argp.h>
#include <signal.h>

#include "tuya_log.h"
#include "tuya_error_code.h"
#include "tuyalink_core.h"

#include "become_daemon.h"
#include "action_handler.h"
#include "interface_info.h"
#include "tuya_actions.h"

static struct argp_option options[] = {
    {"daemon", 'D', 0, 0, "Run as daemon"},
    {"device_id", 'd', "ID", 0, "Specify device id"},
    {"device_secret", 's', "SECRET", 0, "Specify device secret"},
    {"product_id", 'p', "ID", 0, "Specify product id"},
    { 0 }
};

struct arguments
{
    char *d_id;
    char *d_secret;
    char *p_id;
    int daemon;
};

struct arguments arguments;

static char doc[] = "Device connection";

static char args_doc[] = "ARG1 ARG2 ARG3";
static error_t parse_opt (int key, char *arg, struct argp_state *state);
static struct argp argp = { options, parse_opt, args_doc, doc };

int exit_flag = 0;

static void send_info(tuya_mqtt_context_t* context);
static void sig_handler(int signum);
static void map_signal_handling();


int main(int argc, char** argv)
{
    arguments.d_id = "";
    arguments.d_secret = "";
    arguments.p_id = "";
    arguments.daemon = 0;

    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    if (strcmp(arguments.d_id, "") == 0 || strcmp(arguments.d_secret, "") == 0 || strcmp(arguments.p_id, "") == 0) {
        printf("Missing arguments: -d, -s, and -p must all be specified\n");
        exit(EXIT_FAILURE);
    }

    if(arguments.daemon == 1){
        int ret = become_daemon(0);
        if(ret)
        {
            syslog(LOG_ERROR, "error starting");
            closelog();
            return EXIT_FAILURE;
        }
    }

    map_signal_handling();

    tuya_mqtt_context_t* client = malloc(sizeof(tuya_mqtt_context_t));

    if (client == NULL)
        return EXIT_FAILURE;

    int ret = tuya_connect_device(client, arguments.d_id, arguments.d_secret);

    if (ret != OPRT_OK) {
        TY_LOGE("tuya connect failed: %d", ret);
        return ret;
    }

    while (exit_flag == 0)
        main_tuya_loop(client, arguments.d_id, 5);

    tuya_disconnect_device(client);
    free(client);
    return ret;
}

static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
    switch (key) {
        case 'd':
            arguments.d_id = arg;
            break;
        case 's':
            arguments.d_secret = arg;
            break;
        case 'p':
            arguments.p_id = arg;
            break;
        case 'D':
            arguments.daemon = 1;
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static void sig_handler(int signum)
{
    exit_flag = 1;
}

static void map_signal_handling()
{
    int i;
    for (i = 1; i < NSIG; i++) {
        if (i != SIGINT && i != SIGTERM && i != SIGKILL && i != SIGSTOP) {
            signal(i, SIG_IGN);
        }
    }

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGQUIT, sig_handler);
    signal(SIGKILL, SIG_DFL);
    signal(SIGABRT, SIG_DFL);
}