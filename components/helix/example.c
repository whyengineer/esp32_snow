
/***********
author:大约在秋季(qq:814605129)


***************/


/**
 * 使用SDMMC外设访问SD卡，读取mp3文件进行解码并保存到文件
 */


#ifdef CONFIG_AUDIO_HELIX

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/sdmmc_host.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "mp3dec.h"


//关于read_buffer的空间长度应该满足最高比特率条件下的一帧的最大大小(MAINBUF_SIZE = 1940)
static unsigned char read_buffer[MAINBUF_SIZE];


static short int out[1152 * 2];

static void mp3_task(void *args) {
    FILE *fd1 = fopen("/sdcard/2.mp3", "rb");
    if (!fd1) {
        printf("open 2.mp3 error\n");
        vTaskDelete(NULL);
        return;
    }
    FILE *fd2 = fopen("/sdcard/out.pcm", "w");
    if (!fd2) {
        printf("open out.pcm error\n");
        fclose(fd1);
        vTaskDelete(NULL);
        return;
    }
    HMP3Decoder *decoder = MP3InitDecoder(); 
    if (!decoder) {
        fclose(fd1);
        fclose(fd2);
        printf("init mp3 decoder error\n");
        vTaskDelete(NULL);
        return;
    }


	struct stat st; 
	stat("/sdcard/2.mp3", &st);
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
			printf("tag_len: %d\n", tag_len);
			if (tag_len >= size) {
				printf("file error\n");
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
                //printf("sample rate: %d, channels: %d, outputSamps: %d\n", frame_info.samprate, frame_info.nChans, outputSamps);
                if (outputSamps > 0) {
                    if (frame_info.nChans == 1) {
                        int i;
                        for (i = outputSamps - 1; i >= 0; i--) {
                            out[i * 2] = out[i];
                            out[i * 2 + 1] = out[i];
                        }
                        outputSamps *= 2;
                    }
                    fwrite(out, 1, outputSamps * sizeof(short), fd2);
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
    fclose(fd1);
    fclose(fd2);
    printf("exit\n");
    vTaskDelete(NULL);
}


void app_main() {
    printf("Initializing SD card\n");
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    //当前板卡不支持卡监听和写保护信号
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5
    };
    // Use settings defined above to initialize SD card and mount FAT filesystem.
    // Note: esp_vfs_fat_sdmmc_mount is an all-in-one convenience function.
    // Please check its source code and implement error recovery when developing
    // production applications.
    sdmmc_card_t *card;
    esp_err_t err = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);
    if (err != ESP_OK) {
        if (err == ESP_FAIL) {
            printf("Failed to mount filesystem. If you want the card to be formatted, set format_if_mount_failed = true.\n");
        } else {
            printf("Failed to initialize the card (%d). Make sure SD card lines have pull-up resistors in place.\n", err);
        }
        return;
    }
    sdmmc_card_print_info(stdout, card);
    xTaskCreate(mp3_task, "mp3_task", 4096, NULL, 5, NULL);
    //Unmount partition and disable SDMMC or SPI peripheral
    //esp_vfs_fat_sdmmc_unmount();
}

#endif


