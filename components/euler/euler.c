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
#include "euler.h"
#include <math.h>
#include "imuUpdate.h"
#include "websocket.h"

#define TAG "euler"


EulerTypeDef euler_data;
static int16_t gyro_offset[3];


static void gyro_calib();
static void sensor_data_update();
static float CLMAP(float a,float min,float max);
static void  euler2quat();
static void quat2euler();
static void quaternion_init();
static void euler_data_update();


static void update_task(void *pvParameters){
	MPU6050_Initialize();
	gyro_calib();
	quaternion_init();
	while(1){
		euler_data_update();
		vTaskDelay(2);//500hz
	}
}

void euler_task( void *pvParameters ){
	//memset()
	if(MPU6050_TestConnection()!=ESP_OK){
		ESP_LOGI(TAG,"connect failed");
	}else{
		ESP_LOGI(TAG,"mpu6050 connect ok!!!");
	}
	xTaskCreate(&update_task, "euler_update_task", 4096, NULL, 10, NULL);
  cJSON *root=NULL;
  char* out=NULL;
	while(1){
		//sensor_data_update();
    root=cJSON_CreateObject();
    cJSON_AddNumberToObject(root,"roll",(int)(euler_data.euler[0]*57320));
    cJSON_AddNumberToObject(root,"pitch",(int)(euler_data.euler[1]*57320));
    cJSON_AddNumberToObject(root,"yaw",(int)(euler_data.euler[2]*57320));
    out = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    WS_write_data(out,strlen(out));
    free(out);
		//ESP_LOGI(TAG,"\nroll:%f,pitch:%f,yaw:%f",euler_data.euler[0]*57.32,euler_data.euler[1]*57.32,euler_data.euler[2]*57.32);
		vTaskDelay(100);
	}
	//vTaskSuspend(NULL);
}



static void euler_data_update(){
	sensor_data_update();
	IMUupdate();
	quat2euler();
}
static void quat2euler(){
	float q0,q1,q2,q3;
  	float pitch,roll,yaw;
  	q0=euler_data.q[0];
  	q1=euler_data.q[1];
  	q2=euler_data.q[2];
  	q3=euler_data.q[3];
  	//roll=asin(CLMAP(2 * (q2 * q3 + q0 * q1) , -1.0f , 1.0f));
  	//pitch=-atan2(2 * (q1 * q3 - q0* q2) , 1- 2 * (q2 * q2+ q1 * q1));
  	//yaw=atan2(my*arm_cos_f32(roll)+mx*arm_sin_f32(roll)*arm_sin_f32(pitch)-mz*arm_sin_f32(roll)*arm_cos_f32(pitch),mx*arm_cos_f32(pitch)-mz*arm_sin_f32(pitch))*57.3;
  	//yaw = -(0.9 * (-yaw + gz*0.002*57.3) + 5.73* atan2(mx*cos(roll) + my*sin(roll)*sin(pitch) + mz*sin(roll)*cos(pitch), my*cos(pitch) - mz*sin(pitch)));
  	//yaw=atan2(2 * (q0 * q2 + q3 * q1) , 1 - 2 * (q1 * q1 + q2 * q2))*57.3; 
  	//yaw=atan2(mx,my)*57.3;
  
  	//Data.euler[0] = roll; 
  	//Data.euler[1]  = pitch;
  	//Data.euler[2] =0.9*(yaw1+gz*57.3*0.002)+0.1*yaw;
  	//Data.euler[2] = atan2(2 * (q0 * q2 + q3 * q1) , 1 - 2 * (q1 * q1 + q2 * q2)); 
  	//Data.euler[2]=atan2(2*(q1*q2 + q0*q3),q0*q0+q1*q1-q2*q2-q3*q3);
  	//Data.euler[2] =0.9*(yaw-gz*57.3*0.002)+0.1*yaw;
  	//Data.euler[2]=yaw;
  	roll=atan2(2*(q0*q1+q2*q3),1-2*(q1*q1+q2*q2));
  	pitch=asin(CLMAP(2*(q0*q2-q1*q3),-1.0f,1.0f));
  	yaw=atan2(2*(q0*q3+q1*q2),1-2*(q3*q3+q2*q2));
  
  	euler_data.euler[0]=roll; 
  	euler_data.euler[1]=pitch;
  	euler_data.euler[2]=yaw; 
}
static void euler2quat(){
	float recipNorm;
    float fCosHRoll = cos(euler_data.euler[0] * .5f);
    float fSinHRoll = sin(euler_data.euler[0] * .5f);
    float fCosHPitch = cos(euler_data.euler[1] * .5f);
    float fSinHPitch = sin(euler_data.euler[1] * .5f);
    float fCosHYaw = cos(euler_data.euler[2] * .5f);
    float fSinHYaw = sin(euler_data.euler[2]* .5f);
    float q0,q1,q2,q3;

    /// Cartesian coordinate System
    q0 = fCosHRoll * fCosHPitch * fCosHYaw + fSinHRoll * fSinHPitch * fSinHYaw;
    q1 = fSinHRoll * fCosHPitch * fCosHYaw - fCosHRoll * fSinHPitch * fSinHYaw;
    q2 = fCosHRoll * fSinHPitch * fCosHYaw + fSinHRoll * fCosHPitch * fSinHYaw;
    q3 = fCosHRoll * fCosHPitch * fSinHYaw - fSinHRoll * fSinHPitch * fCosHYaw;

    //  q0 = fCosHRoll * fCosHPitch * fCosHYaw - fSinHRoll * fSinHPitch * fSinHYaw;
    //  q1 = fCosHRoll * fSinHPitch * fCosHYaw - fSinHRoll * fCosHPitch * fSinHYaw;
    //  q2 = fSinHRoll * fCosHPitch * fCosHYaw + fCosHRoll * fSinHPitch * fSinHYaw;
    //  q3 = fCosHRoll * fCosHPitch * fSinHYaw + fSinHRoll * fSinHPitch * fCosHYaw;
        
    recipNorm = invSqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
    q0 *= recipNorm;
    q1 *= recipNorm;
    q2 *= recipNorm;
    q3 *= recipNorm;
  
    euler_data.q[0]=q0;
    euler_data.q[1]=q1;
    euler_data.q[2]=q2;
    euler_data.q[3]=q3;      
}
static void quaternion_init(){
	float roll,pitch;
	sensor_data_update();
	roll=atan2(euler_data.accel[1],euler_data.accel[2]);
    pitch=-atan2(euler_data.accel[0],euler_data.accel[2]);
    euler_data.euler[0]=roll;
	euler_data.euler[1]=pitch;
	euler_data.euler[2]=0.0;
	euler2quat();
}
static void sensor_data_update(){
	int16_t buf[6];
	MPU6050_GetRawAccelGyro(buf);
	euler_data.accel[0]=buf[0]/32768.0*2.0*9.8;  //m2/s
	euler_data.accel[1]=buf[1]/32768.0*2.0*9.8;
	euler_data.accel[2]=buf[2]/32768.0*2.0*9.8;
	euler_data.gyro[0]=(buf[3]-gyro_offset[0])/32768.0*250/57.32;//radio
	euler_data.gyro[1]=(buf[4]-gyro_offset[1])/32768.0*250/57.32;
	euler_data.gyro[2]=(buf[5]-gyro_offset[2])/32768.0*250/57.32;
}
static void gyro_calib(){
	int gyro_tmp[3]={0};
	int16_t tmp[6];
	for(int i=0;i<30;i++){
		MPU6050_GetRawAccelGyro(tmp);
		gyro_tmp[0]+=tmp[3];
		gyro_tmp[1]+=tmp[4];
		gyro_tmp[2]+=tmp[5];
		vTaskDelay(100);
	}
	gyro_offset[0]=gyro_tmp[0]/30;
	gyro_offset[1]=gyro_tmp[1]/30;
	gyro_offset[2]=gyro_tmp[2]/30;
}
float CLMAP(float a,float min,float max)
{
  if(a<min)
    a=min;
  else if(a>max)
    a=max;
  else
    a=a;
  return a;
}