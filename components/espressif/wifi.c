

#include <string.h>
#include <sys/socket.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"

#include "wifi.h"
#include "event.h"


#define EXAMPLE_DEFAULT_SSID "esp32"
#define EXAMPLE_DEFAULT_PWD "1234567890"

#define EXAMPLE_MAX_STA_CONN 3 

#define TAG "wifi:"


void wifi_init_sta(char* ssid,char* pw)
{
    //station_event_group = xEventGroupCreate();

    //tcpip_adapter_init();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    // wifi_config_t wifi_config = {
    //     .sta = {
    //         .ssid = ssid,
    //         .password = pw
    //     },
    // };
    wifi_config_t wifi_config;
    memset(&wifi_config,0,sizeof(wifi_config));
    memcpy(wifi_config.sta.ssid,ssid,strlen(ssid));
    memcpy(wifi_config.sta.password,pw,strlen(pw));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");
    ESP_LOGI(TAG, "connect to ap SSID:%s password:%s \n",
        ssid,pw);
}
void wifi_init_softap(char* ssid,char* pw)
{
    //ap_event_group = xEventGroupCreate();

    //tcpip_adapter_init();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    wifi_config_t wifi_config;
    memset(&wifi_config,0,sizeof(wifi_config));

    memcpy(wifi_config.ap.ssid,ssid,strlen(ssid));
    wifi_config.ap.ssid_len=strlen(ssid);
    wifi_config.ap.max_connection=EXAMPLE_MAX_STA_CONN;
    memcpy(wifi_config.ap.password,pw,strlen(pw));
    wifi_config.ap.authmode=WIFI_AUTH_WPA_WPA2_PSK;
    // wifi_config_t wifi_config = {
    //     .ap = {
    //         .ssid = EXAMPLE_DEFAULT_SSID,
    //         .ssid_len = strlen(EXAMPLE_DEFAULT_SSID),
    //         .max_connection=EXAMPLE_MAX_STA_CONN,
    //         .password = EXAMPLE_DEFAULT_PWD,
    //         .authmode = WIFI_AUTH_WPA_WPA2_PSK
    //     },
    // };
    // if (strlen(EXAMPLE_DEFAULT_PWD) ==0) {
       // wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    // }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished.SSID:%s password:%s \n",\
            ssid, pw);
}