#ifndef MYMQTT_H
#define MYMQTT_H

#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "addr_from_stdin.h"
#include "lwip/err.h"
#include "lwip/sockets.h"

/* Protótipos */
int mymqtt_connect(char *host_ip, int port);
void mymqtt_disconnect(void);
int mymqtt_publish(char *topic, void * payload, uint32_t size);
int mymqtt_subscribe(char * topic);
int mymqtt_listen(char * rx_buffer, int buffer_size);

#endif