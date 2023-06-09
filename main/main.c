/* BSD Socket API Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
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

#include "mymqtt.h"

#define HOST_IP_ADDR "20.213.253.93" // IP do servidor Azure
#define PORT 1883                    // Porta de aplicação MQTT
#define TOPIC_NAME "TEMP"            // Nome do tópico

static const char *TAG = "main";

/**
 * @brief Task para Publicar via MQTT
 *
 * @param pvParameters
 */
static void mqtt_pub_task(void *pvParameters)
{
    int ret, count = 0;

    while (1)
    {

        ret = mymqtt_connect(HOST_IP_ADDR, PORT); // Realiza o connect MQTT
        if (ret < 0)
        {
            ESP_LOGE(TAG, "MQTT connect failed, return: %d", ret);
            break;
        }
        else
        {
            ESP_LOGI(TAG, "MQTT connect success, return: %d", ret);
        }

        while (1) // Loop para publicar os dados
        {
            ret = mymqtt_publish("COUNT", &count, sizeof(count));
            if (ret < 0)
            {
                ESP_LOGE(TAG, "MQTT publish failed, return: %d", ret);
                break;
            }
            else
            {
                ESP_LOGI(TAG, "MQTT publish success, return: %d", ret);
            }
            count++;
            vTaskDelay(3000 / portTICK_PERIOD_MS);
            
            if(count > 10) // Encerra o teste
            {
                break;
            }
        }

        mymqtt_disconnect(); // Realiza o disconnect MQTT
    }

    vTaskDelete(NULL);
}

/**
 * @brief Task para gerenciar o MQTT
 *
 * @param pvParameters
 */
static void mqtt_sub_task(void *pvParameters)
{
    int ret, len, count = 0;
    char rx_buffer[100]; // Buffer de recebimento

    while (1)
    {
        ret = mymqtt_connect(HOST_IP_ADDR, PORT); // Realiza o connect MQTT
        if (ret < 0)
        {
            ESP_LOGE(TAG, "MQTT connect failed, return: %d", ret);
            break;
        }
        else
        {
            ESP_LOGI(TAG, "MQTT connect success, return: %d", ret);
        }

        ret = mymqtt_subscribe("ESPTEST"); // Realiza inscrição no tópico
        if (ret < 0)
        {
            ESP_LOGE(TAG, "MQTT subscribe failed, return: %d", ret);
            break;
        }
        else
        {
            ESP_LOGI(TAG, "MQTT subscribe success, return: %d", ret);
        }

        // while (1) // Loop para receber os dados
        // {
        //     ESP_LOGI(TAG, "Waiting for packets");
        //     len = mymqtt_listen(rx_buffer, sizeof(rx_buffer));
        //     count++;
        //     if(len < 0)
        //     {
        //         ESP_LOGI(TAG, "Failed");
        //         break;
        //     }
        //     else
        //     {
        //         ESP_LOGI(TAG, "Received %d bytes", len);
        //         ESP_LOG_BUFFER_HEX(TAG, rx_buffer, len);
        //     }

        //     vTaskDelay(10/portTICK_PERIOD_MS);
            
        //     if(count > 2)
        //     {
        //         count = 0;
        //         break;
        //     }
        // }

        vTaskDelay(5000/portTICK_PERIOD_MS);

        ret = mymqtt_unsubscribe("ESPTEST"); // Desinscreve do tópico
        if (ret < 0)
        {
            ESP_LOGE(TAG, "MQTT unsubscribe failed, return: %d", ret);
        }
        else
        {
            ESP_LOGI(TAG, "MQTT unsubscribe success, return: %d", ret);
        }

        mymqtt_disconnect(); // Realiza o disconnect MQTT
    }

    vTaskDelete(NULL);
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    //xTaskCreate(mqtt_pub_task, "mqtt_pub_task", 4096, NULL, 5, NULL);
    xTaskCreate(mqtt_sub_task, "mqtt_sub_task", 4096, NULL, 5, NULL);
}
