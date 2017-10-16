#ifndef EULER_H
#define EULER_H


typedef struct 
{
	float accel[3];
	float gyro[3];
	float euler[3];
	float q[4];
}EulerTypeDef;

void euler_task( void *pvParameters );



#endif