#ifndef PLAY_H


typedef struct 
{
    char rld[4];    //riff 标志符号
    int  rLen;      //
    char wld[4];    //格式类型（wave）
    char fld[4];    //"fmt"
 
    int fLen;   //sizeof(wave format matex)
 
    short wFormatTag;   //编码格式
    short wChannels;    //声道数
    int   nSamplesPersec;  //采样频率
    int   nAvgBitsPerSample;//WAVE文件采样大小
    short wBlockAlign; //块对齐
    short wBitsPerSample;   //WAVE文件采样大小

    char dld[4];        //”data“
    int  wSampleLength; //音频数据的大小
 }WAV_HEADER;

void aplay_wav(char* filename);
void aplay_mp3(char* filename);


 #endif

