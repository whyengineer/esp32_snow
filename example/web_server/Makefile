#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := ESP32_SNOW_WEB_SERVER


a:=$(shell pwd)
b:=$(dir $(patsubst %/,%,$(dir $(a))))
EXTRA_COMPONENT_DIRS:=$(b)components
	
export EXTRA_COMPONENT_DIRS

include $(IDF_PATH)/make/project.mk


