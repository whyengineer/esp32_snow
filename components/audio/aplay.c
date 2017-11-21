#include "esp_log.h"
#include "esp_err.h"
#include "hal_i2c.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hal_i2s.h"
#include "aplay.h"
#include <sys/stat.h>

#ifdef CONFIG_AUDIO_MAD
  #include "mad.h"
#endif

#ifdef CONFIG_AUDIO_HELIX
  #include "mp3dec.h"
#endif

#define TAG "aplay"

struct file_bufer {
  unsigned char *buf;
  unsigned long length;
  uint32_t flen;
  uint32_t fpos;
  FILE* f;
};

void aplay_wav(char* filename){
	//"/sdcard/test.wav"
	WAV_HEADER wav_head;
	FILE *f= fopen(filename, "r");
    if (f == NULL) {
        ESP_LOGE(TAG,"Failed to open file:%s",filename);
        return;
    }
    //fprintf(f, "Hello %s!\n", card->cid.name);
    int rlen=fread(&wav_head,1,sizeof(wav_head),f);
    if(rlen!=sizeof(wav_head)){
        ESP_LOGE(TAG,"read faliled");
        return;
    }
    int channels = wav_head.wChannels;
    int frequency = wav_head.nSamplesPersec;
    int bit = wav_head.wBitsPerSample;
    int datalen= wav_head.wSampleLength;
    ESP_LOGI(TAG,"channels:%d,frequency:%d,bit:%d\n",channels,frequency,bit);
    char* samples_data = malloc(1024);
   	do{
   		rlen=fread(samples_data,1,1024,f);
   		//datalen-=rlen;
    	hal_i2s_write(0,samples_data,rlen,5000);
    }while(rlen>0);
    fclose(f);
    free(samples_data);
    f=NULL;
}

#ifdef CONFIG_AUDIO_HELIX

void aplay_mp3(char* filename){
  unsigned char *read_buffer=heap_caps_malloc(MAINBUF_SIZE,MALLOC_CAP_SPIRAM);
  if(read_buffer==NULL){
    ESP_LOGE(TAG,"malloc read buf failed");
    return;
  }
  FILE *fd1 = fopen(filename, "rb");
  if (!fd1) {
    heap_caps_free(read_buffer);
    ESP_LOGE(TAG,"open %s error",filename);
    return;
  }
  // FILE *fd2 = fopen("/sdcard/out.pcm", "w");
  // if (!fd2) {
  //     printf("open out.pcm error\n");
  //     fclose(fd1);
  //     vTaskDelete(NULL);
  //     return;
  // }
  HMP3Decoder *decoder = MP3InitDecoder(); 
  if (!decoder) {
      fclose(fd1);
      heap_caps_free(read_buffer);
      ESP_LOGE(TAG,"init mp3 decoder error");
      return;
  }
  short int *out=heap_caps_malloc(1152 * 2,MALLOC_CAP_SPIRAM);
  if(out==NULL){
    fclose(fd1);
    heap_caps_free(read_buffer);
    MP3FreeDecoder(decoder);
    ESP_LOGE(TAG,"malloc out buf failed");
    return;
  }
  struct stat st; 
  stat(filename, &st);
  long long size = st.st_size;
  int eof = 0;
  int read_bytes;
  int offset;
  int left = 0;               //遗留未处理的字节
  int ret;
  char tag[10];
  int tag_len = 0;
  read_bytes = fread(tag, 1, 10, fd1);
  if (read_bytes != 10) {
    eof = 1;
  } else {
    if (strncmp("ID3", tag, 3) == 0) {
      tag_len = ((tag[6] & 0x7F) << 21) |
        ((tag[7] & 0x7F) << 14) |
        ((tag[8] & 0x7F) << 7) | (tag[9] & 0x7F);
      ESP_LOGI(TAG,"tag_len: %d", tag_len);
      if (tag_len >= size) {
        ESP_LOGE(TAG,"file format error");
        eof = 1;
      } else {
        fseek(fd1, tag_len - 10, SEEK_SET);
      }
    } else {
      fseek(fd1, 0, SEEK_SET);
    }
  }   

  while (!eof) {
    read_bytes = fread(read_buffer + left, 1, sizeof(read_buffer) - left, fd1);
    left += read_bytes;
    if (left == 0) break;
    offset = MP3FindSyncWord(read_buffer, left);
    if (offset < 0) {
        //当前缓冲中无同步字，重新读取数据
        left = 0;
        //printf("can not find sync words\n");
        continue;
    }
    if (offset > 0) {
        //去除头部无效数据
        left -= offset;
        memmove(read_buffer, read_buffer + offset, left);
    }
    //printf("sync offset: %d, left: %d\n", offset, left);
    //读满数据缓冲
    unsigned char *read_ptr;
    read_bytes = fread(read_buffer + left, 1, sizeof(read_buffer) - left, fd1);
    left += read_bytes;
    if (left == 0) eof = 1;
    read_ptr = read_buffer;
    ret = MP3Decode(decoder, &read_ptr, &left, out, 0);
    if (ret == ERR_MP3_NONE) {
      int outputSamps;
      MP3FrameInfo frame_info;
      MP3GetLastFrameInfo(decoder, &frame_info);
      /* set sample rate */
      /* write to sound device */
      outputSamps = frame_info.outputSamps;
      ESP_LOGI(TAG,"sample rate: %d, channels: %d, outputSamps: %d\n", frame_info.samprate, frame_info.nChans, outputSamps);
      if (outputSamps > 0) {
          if (frame_info.nChans == 1) {
              int i;
              for (i = outputSamps - 1; i >= 0; i--) {
                  out[i * 2] = out[i];
                  out[i * 2 + 1] = out[i];
              }
              outputSamps *= 2;
          }
          //fwrite(out, 1, outputSamps * sizeof(short), fd2);
      } else {
          //printf("no samples\n");
      }
      memmove(read_buffer, read_ptr, left);
    } else {
      if (ret == ERR_MP3_INDATA_UNDERFLOW) {
          //printf("ERR_MP3_INDATA_UNDERFLOW\n");
          left = 0;
      } else if (ret == ERR_MP3_MAINDATA_UNDERFLOW) {
          /* do nothing - next call to decode will provide more mainData */
          //printf("ERR_MP3_MAINDATA_UNDERFLOW, continue to find sys words, left: %d\n", left);
          if (left > 0) {
              memmove(read_buffer, read_ptr, left);
          }
      } else {
          //printf("unknown error: %d, left: %d\n", ret, left);
          // skip this frame
          if (left > 0) {
              read_ptr++;
              left--;
              memmove(read_buffer, read_ptr, left);
          }
      }
    }

  }
  MP3FreeDecoder(decoder);
  heap_caps_free(out);
  fclose(fd1);
  heap_caps_free(read_buffer);
  ESP_LOGI(TAG,"play file:%s finished",filename);
}
#endif
// static enum mad_flow input_func(void *data, struct mad_stream *stream);
// static enum mad_flow output_func(void *data,struct mad_header const *header,struct mad_pcm *pcm);
// static enum mad_flow error_func(void *data,struct mad_stream *stream,struct mad_frame *frame);
// //static enum mad_flow (*header_func)(void *, struct mad_header const *);
// //static enum mad_flow (*filter_func)(void *, struct mad_stream const *, struct mad_frame *);
// //static enum mad_flow (*message_func)(void *, void *, unsigned int *);

// void aplay_mp3(char* filename){
// 	if(filename==NULL)
// 		return;
// 	struct mad_decoder* decoder=NULL;
//     decoder=malloc(sizeof(struct mad_decoder));
//     if(decoder==NULL){
//         ESP_LOGI(TAG,"malloc mad decoder failed");
//         goto fail;
//     }
//     struct file_bufer mp3_file;
//     mp3_file.buf=malloc(READBUFSZ);
//     if(mp3_file.buf==NULL){
//         ESP_LOGE(TAG,"malloc buf failed");
//         goto fail;
//     }
//     mp3_file.length=READBUFSZ;
//     mp3_file.f= fopen(filename, "r");
//     if (mp3_file.f == NULL) {
//         ESP_LOGE(TAG,"Failed to open file:%s",filename);
//         goto fail;
//     }
//     //get file length
//     fseek (mp3_file.f, 0, SEEK_END);
//     mp3_file.flen=ftell(mp3_file.f);
//     fseek (mp3_file.f, 0, SEEK_SET);
//     ESP_LOGI(TAG,"open mp3 file:%s,length:%d",filename,mp3_file.flen);
//     int result;
//     mad_decoder_init(decoder,&mp3_file,input_func,NULL,NULL,output_func,error_func,NULL);
//     result = mad_decoder_run(decoder, MAD_DECODER_MODE_SYNC);
//     ESP_LOGI(TAG,"decoder result :%d",result);
//     mad_decoder_finish(decoder);
// fail:
//     //ESP_LOGE(TAG,"malloc failed!");
//     if(decoder!=NULL)
//         free(decoder);
//     if(mp3_file.buf!=NULL)
//         free(mp3_file.buf);
//     if(mp3_file.f!=NULL){
//         fclose(mp3_file.f);
//     }
// }
// /*
// fixed point 24 to int
// */
// static inline signed int scale(mad_fixed_t sample)
// {
//       /* round */
//       sample += (1L << (MAD_F_FRACBITS - 16));

//       /* clip */
//       if (sample >= MAD_F_ONE)
//         sample = MAD_F_ONE - 1;
//       else if (sample < -MAD_F_ONE)
//         sample = -MAD_F_ONE;

//       /* quantize */
//       return sample >> (MAD_F_FRACBITS + 1 - 16);
// }

// static enum mad_flow input_func(void *data, struct mad_stream *stream){
//     struct file_bufer *mp3fp;
//     int ret_code;
//     int unproc_data_size; /*the unprocessed data's size*/
//     int copy_size;
//     int size;
//     //ESP_LOGI(TAG,"MAD INPUT");
//     mp3fp = (struct file_bufer *)data;
//     if(mp3fp->fpos < mp3fp->flen) {
//         unproc_data_size = stream->bufend - stream->next_frame;
//         //printf("%d, %d, %d\n", unproc_data_size, mp3fp->fpos, mp3fp->fbsize);
//         memcpy(mp3fp->buf, mp3fp->buf + mp3fp->flen - unproc_data_size, unproc_data_size);
//         copy_size = mp3fp->length - unproc_data_size;
//         if(mp3fp->fpos + copy_size > mp3fp->flen) {
//             copy_size = mp3fp->flen - mp3fp->fpos;
//         }
//         fread(mp3fp->buf+unproc_data_size, 1, copy_size, mp3fp->f);
//         size = unproc_data_size + copy_size;
//         mp3fp->fpos += copy_size;

//         /*Hand off the buffer to the mp3 input stream*/
//         mad_stream_buffer(stream, mp3fp->buf, size);
//         ret_code = MAD_FLOW_CONTINUE;
//     } else {
//         ret_code = MAD_FLOW_STOP;
//     }

//     return ret_code;
// }
// static enum mad_flow error_func(void *data,
// 		    struct mad_stream *stream,
// 		    struct mad_frame *frame)
// {
//     struct file_bufer *mp3_file = data;
//     ESP_LOGE(TAG, "decoding error 0x%04x (%s) at byte offset %u\n",
//         stream->error, mad_stream_errorstr(stream),
//         stream->this_frame - mp3_file->buf);
    
//     /* return MAD_FLOW_BREAK here to stop decoding (and propagate an error) */
    
//     return MAD_FLOW_CONTINUE;
// }
// static enum mad_flow output_func(void *data,
// 		     struct mad_header const *header,
// 		     struct mad_pcm *pcm)
// {
//   unsigned int nchannels, nsamples;
//   mad_fixed_t const *left_ch, *right_ch;

//   /* pcm->samplerate contains the sampling frequency */
//   static int init=0;
//   if(init==0){
//     ESP_LOGI(TAG,"sample rate:%d,channel num:%d",pcm->samplerate,pcm->channels);
//     init=1;
//   }
//   nchannels = pcm->channels;
//   nsamples  = pcm->length;
//   left_ch   = pcm->samples[0];
//   right_ch  = pcm->samples[1];
//   int len=0;     
//   char buf[4];
//   while (nsamples--) {
//     signed int sample;
//     /* output sample(s) in 16-bit signed little-endian PCM */

//     sample = scale(*left_ch++);
//         buf[0]=((sample>>0)&0xff);
//         buf[1]=((sample>>8)&0xff);
//         len+=2;
// //    putchar((sample >> 0) & 0xff);
// //    putchar((sample >> 8) & 0xff);

//     if (nchannels == 2) {
//         sample = scale(*right_ch++);
//         buf[2]=((sample>>0)&0xff);
//         buf[3]=((sample>>8)&0xff);
//         len+=2;
// //      putchar((sample >> 0) & 0xff);
// //      putchar((sample >> 8) & 0xff);
//     }
//     i2s_write_bytes(0,buf, 4, 1000 / portTICK_RATE_MS);
//   }

//   return MAD_FLOW_CONTINUE;
// }




