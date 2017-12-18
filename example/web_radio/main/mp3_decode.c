#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "http.h"
#include "driver/gpio.h"
#include "cJSON.h"
#include "hal_i2s.h"
#include "wm8978.h"
#include "http_parser.h"
#include "url_parser.h"
#include "mad.h"
#include "spiram_fifo.h"

#define TAG "MP3_DECODE:"

/*
 * mp3_decoder.c
 *
 *  Created on: 13.03.2017
 *      Author: michaelboeckling
 */

#include <stdlib.h>
#include <string.h>

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "mad.h"
#include "stream.h"
#include "frame.h"
#include "synth.h"

#include "driver/i2s.h"
#include "spiram_fifo.h"
#include "mp3_decode.h"

#define TAG "decoder"

// The theoretical maximum frame size is 2881 bytes,
// MPEG 2.5 Layer II, 8000 Hz @ 160 kbps, with a padding slot plus 8 byte MAD_BUFFER_GUARD.
#define READBUFSZ 2889
static char readBuf[READBUFSZ]; 
// The theoretical minimum frame size of 24 plus 8 byte MAD_BUFFER_GUARD.
#define MIN_FRAME_SIZE (32)

static long bufUnderrunCt;


static enum  mad_flow input(struct mad_stream *stream) {
	int n, i;
	int rem, fifoLen;
	//Shift remaining contents of buf to the front
	rem=stream->bufend-stream->next_frame;
	memmove(readBuf, stream->next_frame, rem);

	while (rem<sizeof(readBuf)) {
		n=(sizeof(readBuf)-rem); 	//Calculate amount of bytes we need to fill buffer.
		i=spiRamFifoFill();
		if (i<n) n=i; 				//If the fifo can give us less, only take that amount
		if (n==0) {					//Can't take anything?
			//Wait until there is enough data in the buffer. This only happens when the data feed 
			//rate is too low, and shouldn't normally be needed!
//			printf("Buf uflow, need %d bytes.\n", sizeof(readBuf)-rem);
			bufUnderrunCt++;
			//We both silence the output as well as wait a while by pushing silent samples into the i2s system.
			//This waits for about 200mS
			i2s_zero_dma_buffer(0);
		} else {
			//Read some bytes from the FIFO to re-fill the buffer.
			spiRamFifoRead(&readBuf[rem], n);
			rem+=n;
		}
	}

	//Okay, let MAD decode the buffer.
	mad_stream_buffer(stream, (unsigned char*)readBuf, sizeof(readBuf));
	return MAD_FLOW_CONTINUE;
}


//Routine to print out an error
static enum mad_flow error(void *data, struct mad_stream *stream, struct mad_frame *frame) {
    printf("dec err 0x%04x (%s)\n", stream->error, mad_stream_errorstr(stream));
    return MAD_FLOW_CONTINUE;
}



//This is the main mp3 decoding task. It will grab data from the input buffer FIFO in the SPI ram and
//output it to the I2S port.
void mp3_decode_task(void *pvParameters)
{
   
    int ret;
    struct mad_stream *stream;
    struct mad_frame *frame;
    struct mad_synth *synth;

    //Allocate structs needed for mp3 decoding
    stream = malloc(sizeof(struct mad_stream));
    frame = malloc(sizeof(struct mad_frame));
    synth = malloc(sizeof(struct mad_synth));
    //buffer_t *buf = buf_create(MAX_FRAME_SIZE);

    if (stream==NULL) { printf("MAD: malloc(stream) failed\n"); return; }
    if (synth==NULL) { printf("MAD: malloc(synth) failed\n"); return; }
    if (frame==NULL) { printf("MAD: malloc(frame) failed\n"); return; }
    //uint32_t buf_underrun_cnt = 0;

    printf("MAD: Decoder start.\n");

    //Initialize mp3 parts
    mad_stream_init(stream);
    mad_frame_init(frame);
    mad_synth_init(synth);


    while(1) {

        // calls mad_stream_buffer internally
        if (input(stream) == MAD_FLOW_STOP ) {
            break;
        }

        // decode frames until MAD complains
        while(1) {

            // returns 0 or -1
            ret = mad_frame_decode(frame, stream);
            if (ret == -1) {
                if (!MAD_RECOVERABLE(stream->error)) {
                    //We're most likely out of buffer and need to call input() again
                    break;
                }
                error(NULL, stream, frame);
                continue;
            }
            mad_synth_frame(synth, frame);
        }
        // ESP_LOGI(TAG, "RAM left %d", esp_get_free_heap_size());
    }

   // abort:
   //  // avoid noise
   //  i2s_zero_dma_buffer(0);
   //  free(synth);
   //  free(frame);
   //  free(stream);
   //  // clear semaphore for reader task
   //  spiRamFifoReset();

   //  printf("MAD: Decoder stopped.\n");

   //  ESP_LOGI(TAG, "MAD decoder stack: %d\n", uxTaskGetStackHighWaterMark(NULL));
    vTaskDelete(NULL);
}

/* Called by the NXP modifications of libmad. Sets the needed output sample rate. */
static int prevRate;
void set_dac_sample_rate(int rate)
{
	//printf("Rate %d\n", rate);
    // if(rate == prevRate)
    //     return;
    // prevRate = rate;

    // mad_buffer_fmt.sample_rate = rate;
}
static inline signed int scale(mad_fixed_t sample)
{
  /* round */
  sample += (1L << (MAD_F_FRACBITS - 16));

  /* clip */
  if (sample >= MAD_F_ONE)
    sample = MAD_F_ONE - 1;
  else if (sample < -MAD_F_ONE)
    sample = -MAD_F_ONE;

  /* quantize */
  return sample >> (MAD_F_FRACBITS + 1 - 16);
}

/* render callback for the libmad synth */
void render_sample_block(short *short_sample_buff, int no_samples)
{
    
    //ESP_LOGI(TAG,"render_sample_length:%d",len);
  //    while (nsamples--) {
  //   signed int sample;

  //   /* output sample(s) in 16-bit signed little-endian PCM */

  //   sample = scale(*left_ch++);
  //   putchar((sample >> 0) & 0xff);
  //   putchar((sample >> 8) & 0xff);

  //   if (nchannels == 2) {
  //     sample = scale(*right_ch++);
  //     putchar((sample >> 0) & 0xff);
  //     putchar((sample >> 8) & 0xff);
  //   }
  // }
	uint32_t len=no_samples*4;
	for(int i=0;i<no_samples;i++){
		short right=short_sample_buff[i];
		short left=short_sample_buff[i];
		char buf[4];
		memcpy(buf,&right,2);
		memcpy(buf+2,&left,2);
		i2s_write_bytes(0,buf, 4, 1000 / portTICK_RATE_MS);
	}
	
    //render_samples((char*) sample_buff_ch0, len, &mad_buffer_fmt);
    return;
}