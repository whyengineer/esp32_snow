/* GPIO Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "wm8978.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "sdmmc_cmd.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include <sys/socket.h>
#include "nvs.h"
#include "nvs_flash.h"
#include "eth.h"
#include "event.h"
#include "wifi.h"
#include "hal_i2c.h"
#include "hal_i2s.h"
#include "wm8978.h"
#include "webserver.h"
#include "http.h"
#include "cJSON.h"
#include "mdns_task.h"
#include "audio.h"
#include <dirent.h>
#include "esp_heap_caps.h"
#include "euler.h"
#include "websocket.h"
#include "esp_heap_caps.h"
#include "lame.h"
#include "aplay.h"
#include "spiram_fifo.h"

#define TAG "main:"
// typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
// typedef int (*http_cb) (http_parser*);


//char* http_body;

#define GPIO_OUTPUT_IO_0    5
#define GPIO_OUTPUT_PIN_SEL  ((1<<GPIO_OUTPUT_IO_0))


#include <sys/time.h>

extern const uint8_t Sample16kHz_raw_start[] asm("_binary_Sample16kHz_raw_start");
extern const uint8_t Sample16kHz_raw_end[]   asm("_binary_Sample16kHz_raw_end");

void lame_task()
{
 lame_t lame;
 unsigned int sampleRate = 16000;
 short int *pcm_samples, *pcm_samples_end;
 int framesize = 0;
 int num_samples_encoded = 0, total=0, frames=0;
 size_t free8start, free32start;
 int nsamples=1152;
 unsigned char *mp3buf;
 const int mp3buf_size=2000;  //mp3buf_size in bytes = 1.25*num_samples + 7200
 struct timeval tvalBefore, tvalFirstFrame, tvalAfter;

  free8start=heap_caps_get_free_size(MALLOC_CAP_8BIT);
 free32start=heap_caps_get_free_size(MALLOC_CAP_32BIT);
 printf("pre lame_init() free mem8bit: %d mem32bit: %d\n",free8start,free32start);

 mp3buf=malloc(mp3buf_size);

 /* Init lame flags.*/
 lame = lame_init();

 free8start=heap_caps_get_free_size(MALLOC_CAP_8BIT);
 free32start=heap_caps_get_free_size(MALLOC_CAP_32BIT);
 printf("post lame_init() free mem8bit: %d mem32bit: %d\n",free8start,free32start);

 if(!lame) {
           printf("Unable to initialize lame object.\n");
          } else {
           printf("Able to initialize lame object.\n");
          }

 /* set other parameters.*/
 lame_set_VBR(lame, vbr_default);
 lame_set_num_channels(lame, 2);
 lame_set_in_samplerate(lame, sampleRate);
 lame_set_quality(lame, 7);  /* set for high speed and good quality. */
 lame_set_mode(lame, JOINT_STEREO);  /* audio is joint stereo */

// lame_set_out_samplerate(lame, sampleRate);
 printf("Able to set a number of parameters too.\n");


 /* set more internal variables. check for failure.*/
 int initRet = lame_init_params(lame);
 if(initRet < 0) printf("Fail in setting internal parameters with code=%d\n",initRet);
 else printf("OK setting internal parameters\n");


 free8start=heap_caps_get_free_size(MALLOC_CAP_8BIT);
 free32start=heap_caps_get_free_size(MALLOC_CAP_32BIT);
 printf("post lame_init_params() free mem8bit: %d mem32bit: %d\n",free8start,free32start);

// lame_print_config(lame);
// lame_print_internals(lame);

 framesize = lame_get_framesize(lame);
 printf("Framesize = %d\n", framesize);
// assert(framesize <= 1152);

 pcm_samples = (short int *)Sample16kHz_raw_start;
 pcm_samples_end = (short int *)Sample16kHz_raw_end;

 //open a file

    FILE *f= fopen("/sdcard/test2.mp3", "w");
    if (f == NULL) {
        ESP_LOGE(TAG,"Failed to open file");
    }
 gettimeofday (&tvalBefore, NULL);
 ESP_LOGI(TAG,"start");
 while ( pcm_samples_end - pcm_samples > 0)
 //for (int j=0;j<1;j++)
 {
       //  printf("\n=============== lame_encode_buffer_interleaved================ \n");
 /* encode samples. */

      num_samples_encoded = lame_encode_buffer_interleaved(lame, pcm_samples, nsamples, mp3buf, mp3buf_size);

  //   printf("number of samples encoded = %d pcm_samples %p \n", num_samples_encoded, pcm_samples);

      if (total==0) gettimeofday (&tvalFirstFrame, NULL);

     /* check for value returned.*/
     if(num_samples_encoded > 1) {
         fwrite(mp3buf,1,num_samples_encoded,f);
       //printf("It seems the conversion was successful.\n");
         total+=num_samples_encoded;
     } else if(num_samples_encoded == -1) {

       printf("mp3buf was too small\n");
       return ;
     } else if(num_samples_encoded == -2) {
       printf("There was a malloc problem.\n");
       return ;
     } else if(num_samples_encoded == -3) {
       printf("lame_init_params() not called.\n");
       return ;
     } else if(num_samples_encoded == -4) {
       printf("Psycho acoustic problems.\n");
       return ;
     } else {
       printf("The conversion was not successful.\n");
       return ;
     }


    // printf("Contents of mp3buffer = ");
/*     for(int i = 0; i < num_samples_encoded; i++) {
         printf("%02X", mp3buf[i]);
     } */

    pcm_samples += (nsamples*2);  // nsamples*2 ????
    frames++;

#if 0
    // to test infinite loop
    if ( pcm_samples_end - pcm_samples <= 0){
        pcm_samples = (short int *)Sample16kHz_raw_start;
        free8start=xPortGetFreeHeapSizeCaps(MALLOC_CAP_8BIT);
        free32start=xPortGetFreeHeapSizeCaps(MALLOC_CAP_32BIT);
        printf("LOOP: free mem8bit: %d mem32bit: %d frames encoded: %d bytes:%d\n",free8start,free32start,frames,total);
    }
#endif
 }
 fclose(f);
 gettimeofday (&tvalAfter, NULL);
 ESP_LOGI(TAG,"done");
 printf("Fist Frame time in microseconds: %ld microseconds\n",
             ((tvalFirstFrame.tv_sec - tvalBefore.tv_sec)*1000000L
            +tvalFirstFrame.tv_usec) - tvalBefore.tv_usec
           );


 printf("Total time in microseconds: %ld microseconds\n",
             ((tvalAfter.tv_sec - tvalBefore.tv_sec)*1000000L
            +tvalAfter.tv_usec) - tvalBefore.tv_usec
           );

 printf ("Total frames: %d TotalBytes: %d\n", frames, total);

/*
 num_samples_encoded = lame_encode_flush(lame, mp3buf, mp3buf_size);
 if(num_samples_encoded < 0) {
   if(num_samples_encoded == -1) {
     printf("mp3buffer is probably not big enough.\n");
   } else {
     printf("MP3 internal error.\n");
   }
   return ;
 } else {
     for(int i = 0; i < num_samples_encoded; i++) {
       printf("%02X ", mp3buf[i]);
     }
     total += num_samples_encoded;
  // printf("Flushing stage yielded %d frames.\n", num_samples_encoded);
 }
*/

// =========================================================

 lame_close(lame);
 printf("\nClose\n");

 while (1) vTaskDelay(500 / portTICK_RATE_MS);

 return;
}
void read_data_task(){
    //open pcm file
    WAV_HEADER wav_head;
    FILE *f= fopen("/sdcard/music.wav", "r");
    if (f == NULL) {
        ESP_LOGE(TAG,"Failed to open file");
    }
    //fprintf(f, "Hello %s!\n", card->cid.name);
    int rlen=fread(&wav_head,1,sizeof(wav_head),f);
    if(rlen!=sizeof(wav_head)){
            ESP_LOGE(TAG,"read faliled");
    }
    int channels = wav_head.wChannels;
    int samplerate= wav_head.nSamplesPersec;
    int bit = wav_head.wBitsPerSample;
    int datalen= wav_head.wSampleLength;
    ESP_LOGI(TAG,"channels:%d,frequency:%d,bit:%d,datalen:%d",channels,samplerate,bit,datalen);
    //init fifo
    spiRamFifoInit();
    //write to fifo
    char* samples_data = malloc(1024);
    do{
        rlen=fread(samples_data,1,1024,f);
        //datalen-=rlen;
        spiRamFifoWrite(samples_data,rlen);
    }while(rlen>0);
    fclose(f);
    free(samples_data);
    f=NULL;
}
void app_main()
{
    esp_err_t err;
    // event_engine_init();
    // nvs_flash_init();
    // tcpip_adapter_init();
    // //wifi_init_sta("Transee21_TP1","02197545");
    // wifi_init_softap("we","1234567890");
    /*init gpio*/
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    gpio_set_level(GPIO_OUTPUT_IO_0, 0);
    /*init sd card*/
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = true,
        .max_files = 10
    };
    sdmmc_card_t* card;
    err = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);
    if (err != ESP_OK) {
        if (err == ESP_FAIL) {
            printf("Failed to mount filesystem. If you want the card to be formatted, set format_if_mount_failed = true.");
        } else {
            printf("Failed to initialize the card (%d). Make sure SD card lines have pull-up resistors in place.", err);
        }
        return;
    }
    sdmmc_card_print_info(stdout, card);
    //xTaskCreate(read_data_task, "read_data_task", 2048, NULL, 6, NULL);
    xTaskCreate(lame_task, "lame_task", 4096*10, NULL, 5, NULL);
    vTaskSuspend(NULL);
    while(1){
    }


}

