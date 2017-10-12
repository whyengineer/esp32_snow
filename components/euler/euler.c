#include "mpu6050.h"
/* Standard includes. */
#include "string.h"
#include "esp_err.h"
/* lwIP core includes */
#include "lwip/opt.h"
#include "lwip/sockets.h"
#include <netinet/in.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "sdmmc_cmd.h"
/* Utils includes. */
#include "esp_log.h"
#include "event.h"
#include "cJSON.h"
#include <dirent.h>

#define TAG "euler"

void euler_task( void *pvParameters ){
	int16_t buf[6];
	if(MPU6050_TestConnection()!=ESP_OK){
		ESP_LOGI(TAG,"connect failed");
	}else{
		ESP_LOGI(TAG,"mpu6050 connect ok!!!");
	}
	MPU6050_Initialize();
	while(1){
		MPU6050_GetRawAccelGyro(buf);
		ESP_LOGI(TAG,"\naccel_x:%d,accel_y:%d,accel_z:%d\ngyro_x:%d,gyro_y:%d,gyro_z:%d",\
			buf[0],buf[1],buf[2],buf[3],buf[4],buf[5]);
		vTaskDelay(1000);
	}
	vTaskSuspend(NULL);
}

