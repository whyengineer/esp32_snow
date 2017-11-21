# : CFLAGS += -Wno-unused-but-set-variable


CFLAGS += -DSTDC_HEADERS -DHAVE_MEMCPY -Duint32_t="unsigned int"\
-Dieee754_float32_t="float" -Duint16_t="unsigned short"\
-Duint8_t="unsigned char" -Dint16_t="signed short"

COMPONENT_ADD_INCLUDEDIRS := include libmp3lame 

COMPONENT_SRCDIRS:=libmp3lame