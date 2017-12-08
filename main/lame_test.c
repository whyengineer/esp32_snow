#include "lame.h"
#include "lame_test.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include <assert.h>
#include <esp_types.h>
#include <stdio.h>
#include "rom/ets_sys.h"
#include "esp_heap_caps.h"
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>


void lameTest()
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

  FILE *fd1 = fopen("/sdcard/in.pcm", "rb");
  if (!fd1) {
      printf("open in.pcm failed");
      fclose(fd1);
      lame_close(lame);
      return;
  }
  FILE *fd2 = fopen("/sdcard/out.mp3", "w");
  if (!fd2) {
      printf("open out.mp3 failed");
      fclose(fd2);
      fclose(fd1);
      lame_close(lame);
      return;
  }
  struct stat st; 
  stat("/sdcard/in.pcm", &st);
  long long file_size = st.st_size;

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


   

  // lame_print_config(lame);
  // lame_print_internals(lame);

   framesize = lame_get_framesize(lame);
   printf("Framesize = %d\n", framesize);
  // assert(framesize <= 1152);

   read_buf =malloc(nsamples);



   free8start=heap_caps_get_free_size(MALLOC_CAP_8BIT);
   free32start=heap_caps_get_free_size(MALLOC_CAP_32BIT);
   printf("post lame_init_params() free mem8bit: %d mem32bit: %d\n",free8start,free32start);

   gettimeofday (&tvalBefore, NULL);
   uint32_t read_l=0;
   while ( file_size> 0)
   //for (int j=0;j<1;j++)
   {
  	   //  printf("\n=============== lame_encode_buffer_interleaved================ \n");
   /* encode samples. */
      read_l= fread(read_buffer, 1, nsamples, fd1);
      file_size-=read_l;//reset file length
      if(read_l==0)
        break;
  	  num_samples_encoded = lame_encode_buffer_interleaved(lame, pcm_samples, nsamples, mp3buf, mp3buf_size);
      
    //   printf("number of samples encoded = %d pcm_samples %p \n", num_samples_encoded, pcm_samples);

  	  if (total==0) gettimeofday (&tvalFirstFrame, NULL);

       /* check for value returned.*/
       if(num_samples_encoded > 1) {
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

   gettimeofday (&tvalAfter, NULL);

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