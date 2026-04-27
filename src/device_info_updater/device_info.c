#include <sys/sysinfo.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <net/if.h>

#include "tuyalink_core.h"
#include "device_info.h"

extern unsigned long get_total_ram()
{
    unsigned long ram = 0;
    struct sysinfo info;
    if (sysinfo(&info) == 0){
        ram = info.totalram/1048576;
    }
    return ram;
}

extern unsigned long get_free_ram()
{
    unsigned long ram = 0;
    struct sysinfo info;
    if (sysinfo(&info) == 0){
        ram = info.freeram/1048576;
    }
    return ram;
}

extern long get_uptime()
{
    long uptime = 0;
    struct sysinfo info;
    if (sysinfo(&info) == 0){
        uptime = info.uptime/60;
    }
    return uptime;
}

extern int get_cpu_usage()
{
    FILE* file;
	char buffer[1024] = "";
	float load;

	file = fopen("/proc/loadavg", "r");
	if(file == NULL) {
		return 0; 
    }

    fread(buffer, 1024, 1, file);
	sscanf(buffer, "%f", &load);

	fclose(file);
	return (int)(load * 100);
}