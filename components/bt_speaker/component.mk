#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

ifdef CONFIG_BT_ENABLED
CFLAGS+=-DCONFIG_BT_ENABLED
COMPONENT_OBJS:=bt_app_av.o bt_app_core.o bt_speaker.o
endif