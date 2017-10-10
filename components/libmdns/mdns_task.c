/* MDNS-SD Query and advertise Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "mdns.h"

/* The examples use simple WiFi configuration that you can set via
   'make menuconfig'.

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/


#define EXAMPLE_MDNS_HOSTNAME "esp32"
#define EXAMPLE_MDNS_INSTANCE "esp32 mdns"


static const char *TAG = "mdns-task";

static void query_mdns_service(mdns_server_t * mdns, const char * service, const char * proto)
{
    if(!mdns) {
        return;
    }
    uint32_t res;
    if (!proto) {
        ESP_LOGI(TAG, "Host Lookup: %s", service);
        res = mdns_query(mdns, service, 0, 1000);
        if (res) {
            size_t i;
            for(i=0; i<res; i++) {
                const mdns_result_t * r = mdns_result_get(mdns, i);
                if (r) {
                    ESP_LOGI(TAG, "  %u: " IPSTR " " IPV6STR, i+1, 
                        IP2STR(&r->addr), IPV62STR(r->addrv6));
                }
            }
            mdns_result_free(mdns);
        } else {
            ESP_LOGI(TAG, "  Not Found");
        }
    } else {
        ESP_LOGI(TAG, "Service Lookup: %s.%s ", service, proto);
        res = mdns_query(mdns, service, proto, 1000);
        if (res) {
            size_t i;
            for(i=0; i<res; i++) {
                const mdns_result_t * r = mdns_result_get(mdns, i);
                if (r) {
                    ESP_LOGI(TAG, "  %u: %s \"%s\" " IPSTR " " IPV6STR " %u %s", i+1, 
                        (r->host)?r->host:"", (r->instance)?r->instance:"", 
                        IP2STR(&r->addr), IPV62STR(r->addrv6),
                        r->port, (r->txt)?r->txt:"");
                }
            }
            mdns_result_free(mdns);
        }
    }
}

void mdns_task(void *pvParameters)
{
    mdns_server_t * mdns = NULL;
    while(1) {
        ESP_LOGI(TAG, "start mdns");
        if (!mdns) {
            esp_err_t err = mdns_init(TCPIP_ADAPTER_IF_AP, &mdns);
            if (err) {
                ESP_LOGE(TAG, "Failed starting MDNS: %u", err);
                continue;
            }

            ESP_ERROR_CHECK( mdns_set_hostname(mdns, EXAMPLE_MDNS_HOSTNAME) );
            ESP_ERROR_CHECK( mdns_set_instance(mdns, EXAMPLE_MDNS_INSTANCE) );

            const char * arduTxtData[4] = {
                "board=esp32",
                "tcp_check=no",
                "ssh_upload=no",
                "auth_upload=no"
            };

            ESP_ERROR_CHECK( mdns_service_add(mdns, "_arduino", "_tcp", 3232) );
            ESP_ERROR_CHECK( mdns_service_txt_set(mdns, "_arduino", "_tcp", 4, arduTxtData) );
            ESP_ERROR_CHECK( mdns_service_add(mdns, "_http", "_tcp", 80) );
            ESP_ERROR_CHECK( mdns_service_instance_set(mdns, "_http", "_tcp", "ESP32 WebServer") );
            ESP_ERROR_CHECK( mdns_service_add(mdns, "_smb", "_tcp", 445) );
            ESP_LOGI(TAG, "init");
        } else {
            query_mdns_service(mdns, "esp32", NULL);
            query_mdns_service(mdns, "_arduino", "_tcp");
            query_mdns_service(mdns, "_http", "_tcp");
            query_mdns_service(mdns, "_printer", "_tcp");
            query_mdns_service(mdns, "_ipp", "_tcp");
            query_mdns_service(mdns, "_afpovertcp", "_tcp");
            query_mdns_service(mdns, "_smb", "_tcp");
            query_mdns_service(mdns, "_ftp", "_tcp");
            query_mdns_service(mdns, "_nfs", "_tcp");
            ESP_LOGI(TAG, "query");
        }

        ESP_LOGI(TAG, "Restarting in 10 seconds!");
        //vTaskDelay(10000 / portTICK_PERIOD_MS);
        vTaskSuspend(NULL);
        ESP_LOGI(TAG, "Starting again!");
    }
}

