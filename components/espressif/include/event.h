#ifndef _EVENT_H
#define _EVENT_H


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#define STA_CONNECTED_BIT BIT0
#define STA_GOTIP_BIT  BIT1

#define AP_START_BIT BIT0

#define ETH_CONNECTED_BIT BIT0
#define ETH_GOTIP_BIT BIT1
#define ETH_DISCONNECTED_BIT BIT2

extern EventGroupHandle_t station_event_group;//24bit
extern EventGroupHandle_t ap_event_group;//24bit
extern EventGroupHandle_t eth_event_group;//24bit


void event_engine_init();
#endif