//event engine
#include "event.h"
#include "esp_system.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_wifi.h"



#define TAG "event:"

EventGroupHandle_t station_event_group;//24bit
EventGroupHandle_t ap_event_group;//24bit
EventGroupHandle_t eth_event_group;//24bit

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:// station start
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED: //station disconnect from ap
        esp_wifi_connect();
        xEventGroupClearBits(station_event_group, STA_CONNECTED_BIT);
        xEventGroupClearBits(station_event_group, STA_GOTIP_BIT);
        break;
    case SYSTEM_EVENT_STA_CONNECTED: //station connect to ap
    	xEventGroupSetBits(station_event_group, STA_CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_GOT_IP:  //station get ip
    	ESP_LOGI(TAG, "got ip:%s\n",
		ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
    	xEventGroupSetBits(station_event_group, STA_GOTIP_BIT);
        break;
    case SYSTEM_EVENT_AP_START:
        xEventGroupSetBits(ap_event_group, AP_START_BIT);
    case SYSTEM_EVENT_AP_STACONNECTED:// a station connect to ap
    	ESP_LOGI(TAG, "station:"MACSTR" join,AID=%d\n",
		MAC2STR(event->event_info.sta_connected.mac),
		event->event_info.sta_connected.aid);
    	//xEventGroupSetBits(tcp_event_group, WIFI_CONNECTED_BIT);
    	break;
    case SYSTEM_EVENT_AP_STADISCONNECTED://a station disconnect from ap
    	ESP_LOGI(TAG, "station:"MACSTR"leave,AID=%d\n",
		MAC2STR(event->event_info.sta_disconnected.mac),
		event->event_info.sta_disconnected.aid);
    	//xEventGroupClearBits(tcp_event_group, WIFI_CONNECTED_BIT);
    	break;
    case SYSTEM_EVENT_ETH_CONNECTED:
    	xEventGroupSetBits(eth_event_group, ETH_CONNECTED_BIT);
    	break;
    case SYSTEM_EVENT_ETH_DISCONNECTED:
    	//xEventGroupClearBits(eth_event_group, ETH_CONNECTED_BIT);
    	//xEventGroupClearBits(eth_event_group, ETH_GOTIP_BIT);
        xEventGroupSetBits(eth_event_group, ETH_DISCONNECTED_BIT);
    case  SYSTEM_EVENT_ETH_GOT_IP:
    	xEventGroupSetBits(eth_event_group, ETH_GOTIP_BIT);
    default:
        break;
    }
    return ESP_OK;
}


void event_engine_init(){
    station_event_group = xEventGroupCreate();
    ap_event_group = xEventGroupCreate();
    eth_event_group = xEventGroupCreate();
	esp_event_loop_init(event_handler,NULL);
}


