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
#include "hal_i2s.h"
#include "audio.h"
#include "wm8978.h"

#define TAG "audiostream:"

static int32_t socket_fd, client_fd;
static struct sockaddr_in server, client;

static int creat_socket_server(in_port_t in_port, in_addr_t in_addr)
{
  int socket_fd, on;
  //struct timeval timeout = {10,0};

  server.sin_family = AF_INET;
  server.sin_port = in_port;
  server.sin_addr.s_addr = in_addr;

  if((socket_fd = socket(AF_INET, SOCK_STREAM, 0))<0) {
    perror("listen socket uninit\n");
    return -1;
  }
  on=1;
  //setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
  setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int) );
  //CALIB_DEBUG("on %x\n", on);
  if((bind(socket_fd, (struct sockaddr *)&server, sizeof(server)))<0) {
    perror("cannot bind srv socket\n");
    return -1;
  }

  if(listen(socket_fd, 1)<0) {
    perror("cannot listen");
    close(socket_fd);
    return -1;
  }

  return socket_fd;
}

void audiostream_task( void *pvParameters ){
	int32_t lBytes;
	esp_err_t err;
	ESP_LOGI(TAG,"audiostream start");
	uint32_t request_cnt=0;
	//init gpio
	(void) pvParameters;
	//init codec
	hal_i2c_init(0,5,17);
    hal_i2s_init(0,16000,16,2); //8k 16bit 1 channel
    i2s_stop(0);
    WM8978_Init();
    WM8978_ADDA_Cfg(1,1); 
    WM8978_Input_Cfg(1,0,0);     
    WM8978_Output_Cfg(1,0); 
    WM8978_MIC_Gain(35);
    WM8978_AUX_Gain(0);
    WM8978_LINEIN_Gain(0);
    WM8978_SPKvol_Set(0);
    WM8978_HPvol_Set(20,20);
    WM8978_EQ_3D_Dir(1);
    WM8978_EQ1_Set(0,24);
    WM8978_EQ2_Set(0,24);
    WM8978_EQ3_Set(0,24);
    WM8978_EQ4_Set(0,24);
    WM8978_EQ5_Set(0,0);
    socklen_t client_size=sizeof(client);
	socket_fd = creat_socket_server(htons(3000),htonl(INADDR_ANY));
	if( socket_fd >= 0 ){
		/* Obtain the address of the output buffer.  Note there is no mutual
		exclusion on this buffer as it is assumed only one command console
		interface will be used at any one time. */
		char* send_buf=NULL;
		send_buf=malloc(1024);
		esp_err_t nparsed = 0;
		/* Ensure the input string starts clear. */
		for(;;){
			client_fd=accept(socket_fd,(struct sockaddr*)&client,&client_size);
			if(client_fd>0L){
				ESP_LOGI(TAG,"start record");
				i2s_start(0);
				// strcat(outbuf,pcWelcomeMessage);
				// strcat(outbuf,path);
				// lwip_send( lClientFd, outbuf, strlen(outbuf), 0 );
				do{
					hal_i2s_read(0,send_buf,1024,portMAX_DELAY);
					lBytes = write( client_fd, send_buf,1024);	
					//while(xReturned!=pdFALSE);
					//lwip_send( lClientFd,path,strlen(path), 0 );
				}while(lBytes > 0);	
			}
			ESP_LOGI(TAG,"request_cnt:%d,socket:%d",request_cnt++,client_fd);
			close( client_fd );
			i2s_stop(0);
		}
	}

}