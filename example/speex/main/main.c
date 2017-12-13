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
 #include <speex/speex.h>
#include "spiram_fifo.h"

#define TAG "main:"
// typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
// typedef int (*http_cb) (http_parser*);


//char* http_body;

#define GPIO_OUTPUT_IO_0    5
#define GPIO_OUTPUT_PIN_SEL  ((1<<GPIO_OUTPUT_IO_0))


#include <sys/time.h>


#define FRAME_SIZE 160
void speex_decode_task(){
  ESP_LOGI(TAG,"start decode");
  FILE *fin;
  FILE *fout;

  /*Holds the audio that will be written to file (16 bits per sample)*/
  short out[FRAME_SIZE];
  /*Speex handle samples as float, so we need an array of floats*/
  float output[FRAME_SIZE];
  char cbits[200];
  int nbBytes;
  /*Holds the state of the decoder*/
  void *state;
  /*Holds bits so they can be read and written to by the Speex routines*/
  SpeexBits bits;
  int i, tmp;
  
  /*Create a new decoder state in narrowband mode*/
  state = speex_decoder_init(&speex_nb_mode);
  /*Set the perceptual enhancement on*/
  tmp=1;
  speex_decoder_ctl(state, SPEEX_SET_ENH, &tmp);

  fin = fopen("/sdcard/speex.spx", "r");
  fout = fopen("/sdcard/newrawpcm.wav", "w");

  /*Initialization of the structure that holds the bits*/
  speex_bits_init(&bits);
  nbBytes=38;
  while(1){
    fread(cbits, 1, nbBytes, fin);
    if (feof(fin))
      break;
    /*Copy the data into the bit-stream struct*/
    speex_bits_read_from(&bits, cbits, nbBytes);
    /*Decode the data*/
    ret=speex_decode(state, &bits, output);
    // ESP_LOGI(TAG,"decode code:%d",ret);
    /*Copy from float to short (16 bits) for output*/
    for (i=0;i<FRAME_SIZE;i++)
      out[i]=output[i];
    fwrite(out, sizeof(short), FRAME_SIZE, fout);
  }
  /*Destroy the decoder state*/
  speex_decoder_destroy(state);
  /*Destroy the bit-stream truct*/
  speex_bits_destroy(&bits);
  fclose(fout);
  fclose(fin);
  ESP_LOGI(TAG,"decode done");
  vTaskDelete(NULL);
}
void speex_encode_task(){
  ESP_LOGI(TAG,"start encode");
  FILE *fin;
  FILE *fout;
  short in[FRAME_SIZE];
  float input[FRAME_SIZE];
  char cbits[200];
  int nbBytes;
  /*Holds the state of the encoder*/
  void *state;
  /*Holds bits so they can be read and written to by the Speex routines*/
  SpeexBits bits;
  int i, tmp;
  /*Create a new encoder state in narrowband mode*/
  state = speex_encoder_init(&speex_nb_mode);
  /*Set the quality to 8 (15 kbps)*/
  tmp=8;
  speex_encoder_ctl(state, SPEEX_SET_QUALITY, &tmp);
  fin = fopen("/sdcard/rawpcm.wav", "r");
  fout = fopen("/sdcard/speex.spx", "w");
  /*Initialization of the structure that holds the bits*/
  speex_bits_init(&bits);

  while(1){
    fread(in, sizeof(short), FRAME_SIZE, fin);
    if(feof(fin))
      break;
    /*Copy the 16 bits values to float so Speex can work on them*/
    for (i=0;i<FRAME_SIZE;i++)
      input[i]=in[i];
    /*Flush all the bits in the struct so we can encode a new frame*/
    speex_bits_reset(&bits);
    speex_encode(state, input, &bits);

    nbBytes = speex_bits_write(&bits, cbits, 200);
    /*Write the size of the frame first. This is what sampledec expects but
    it’s likely to be different in your own application*/
    /*Write the compressed data*/
    fwrite(cbits, 1, nbBytes, fout);
  }
  /*Destroy the encoder state*/
  speex_encoder_destroy(state);
  /*Destroy the bit-packing struct*/
  speex_bits_destroy(&bits);
  fclose(fin);
  fclose(fout);
  ESP_LOGI(TAG,"encode done");
  xTaskCreate(speex_decode_task, "speex_task_decode", 4096*2, NULL, 5, NULL);
  vTaskDelete(NULL);
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
    xTaskCreate(speex_decode_task, "speex_task_encode", 4096*2, NULL, 5, NULL);
    vTaskSuspend(NULL);
    while(1){
    }


}

