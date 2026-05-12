REPO_DIR := ../../../..
COMMON_SRC_DIR := $(REPO_DIR)/sources
MAKE_SYSTEM_PATH := $(REPO_DIR)/make_system

include $(MAKE_SYSTEM_PATH)/mk_tools.mk
PATH:=$(MAKE_SYSTEM_PATH)\tools\win;$(PATH)

-include ../../../../../local_settings.mk


ifneq ($(findstring Windows,$(OS)),)
    EXTENDED_VERSION := $(shell git describe --dirty --always --tags 2>NUL)
else
    EXTENDED_VERSION := $(shell git describe --dirty --always --tags 2>/dev/null)
endif


format: format_all


#Format all source files
FORMAT_DIRS_RECURSIVE :=
FORMAT_DIRS_RECURSIVE += ../../../../sources/stratula/library
FORMAT_DIRS_RECURSIVE += ../../../../sources/stratula/target
FORMAT_DIRS_RECURSIVE += ../../../../sources/stratula/tests
FORMAT_DIRS_RECURSIVE += ../../target_platform/impl
FORMAT_DIRS_RECURSIVE += ../../target_platform/tests
FORMAT_DIRS_RECURSIVE += bsp

FORMAT_DIRS :=
FORMAT_DIRS += sources
FORMAT_DIRS += .

FORMAT_EXTENSIONS = *.c *.h

#Create target format_all from above variables
include $(MAKE_SYSTEM_PATH)/mk_format.mk
