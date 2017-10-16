//=====================================================================================================
// IMU.c
// S.O.H. Madgwick
// 25th September 2010
//=====================================================================================================
// Description:
//
// Quaternion implementation of the 'DCM filter' [Mayhony et al].
//
// User must define 'halfT' as the (sample period / 2), and the filter gains 'Kp' and 'Ki'.
//
// Global variables 'q0', 'q1', 'q2', 'q3' are the quaternion elements representing the estimated
// orientation.  See my report for an overview of the use of quaternions in this application.
//
// User must call 'IMUupdate()' every sample period and parse calibrated gyroscope ('gx', 'gy', 'gz')
// and accelerometer ('ax', 'ay', 'ay') data.  Gyroscope units are radians/second, accelerometer
// units are irrelevant as the vector is normalised.
//
//=====================================================================================================

//----------------------------------------------------------------------------------------------------
// Header files

#include "imuUpdate.h"
#include <math.h>
#include "euler.h"

//----------------------------------------------------------------------------------------------------
// Definitions

#define Kp 2.0f                         // proportional gain governs rate of convergence to accelerometer/magnetometer
#define Ki 0.005f                       // integral gain governs rate of convergence of gyroscope biases
#define halfT 0.001f                    // half the sample period

//---------------------------------------------------------------------------------------------------
// Variable definitions


static float exInt = 0, eyInt = 0, ezInt = 0;        // scaled integral error



//====================================================================================================
// Function
//====================================================================================================
extern EulerTypeDef euler_data;

void IMUupdate() {
        float norm;
        float vx, vy, vz;
        float ex, ey, ez;  
        float ax,ay,az,gx,gy,gz;
        float q0,q1,q2,q3;
        float tq0,tq1,tq2,tq3;
       
        ax=euler_data.accel[0];
        ay=euler_data.accel[1];
        az=euler_data.accel[2];
        
        gx=euler_data.gyro[0];
        gy=euler_data.gyro[1];
        gz=euler_data.gyro[2];
        
        q0=euler_data.q[0];
        q1=euler_data.q[1];
        q2=euler_data.q[2];
        q3=euler_data.q[3];
        
        // normalise the measurements
        norm = sqrt(ax*ax + ay*ay + az*az);      
        ax = ax / norm;
        ay = ay / norm;
        az = az / norm;      

       

        // estimated direction of gravity
        vx = 2*(q1*q3 - q0*q2);
        vy = 2*(q0*q1 + q2*q3);
        vz = q0*q0 - q1*q1 - q2*q2 + q3*q3;



        // error is sum of cross product between reference direction of field and direction measured by sensor
        ex = (ay*vz - az*vy);
        ey = (az*vx - ax*vz);
        ez = (ax*vy - ay*vx);




        // integral error scaled integral gain
        exInt = exInt + ex*Ki;
        eyInt = eyInt + ey*Ki;
        ezInt = ezInt + ez*Ki;

        // adjusted gyroscope measurements
        gx = gx + Kp*ex + exInt;
        gy = gy + Kp*ey + eyInt;
        gz = gz + Kp*ez + ezInt;

        tq0=q0;
        tq1=q1;
        tq2=q2;
        tq3=q3;



        // integrate quaternion rate and normalise
        q0 = tq0 + (-tq1*gx - tq2*gy - tq3*gz)*halfT;
        q1 = tq1 + (tq0*gx + tq2*gz - tq3*gy)*halfT;
        q2 = tq2 + (tq0*gy - tq1*gz + tq3*gx)*halfT;
        q3 = tq3 + (tq0*gz + tq1*gy - tq2*gx)*halfT;  

       

        // normalise quaternion
        norm = invSqrt(q0*q0 + q1*q1 + q2*q2 + q3*q3);
        q0 = q0 * norm;
        q1 = q1 * norm;
        q2 = q2 * norm;
        q3 = q3 * norm;
        
        euler_data.q[0]=q0;
        euler_data.q[1]=q1;
        euler_data.q[2]=q2;
        euler_data.q[3]=q3;
}
float invSqrt(float x) {
	float halfx = 0.5f * x;
	float y = x;
	long i = *(long*)&y;
	i = 0x5f3759df - (i>>1);
	y = *(float*)&i;
	y = y * (1.5f - (halfx * y * y));
	return y;
}
//====================================================================================================
// END OF CODE
//====================================================================================================