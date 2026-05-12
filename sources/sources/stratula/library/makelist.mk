
PARTS ?= components modules protocol remote
PARTS:= $(call lowercase,$(PARTS))


# 1. Determine the current directory
CURRENT_DIR := $(call current_dir)


# 2. Add local C files, includes and flags

C_FILES += $(call rwildcard,$(CURRENT_DIR)/common,*.c)

ifneq ($(filter components,$(PARTS)),)
    C_FILES += $(call rwildcard,$(CURRENT_DIR)/components,*.c)
endif
ifneq ($(filter modules,$(PARTS)),)
    C_FILES += $(call rwildcard,$(CURRENT_DIR)/modules,*.c)
endif
ifneq ($(filter protocol%,$(PARTS)),)
    C_FILES += $(call rwildcard,$(CURRENT_DIR)/protocol/requests,*.c)
endif
ifneq ($(filter protocol,$(PARTS)),)
    C_FILES += $(call rwildcard,$(CURRENT_DIR)/protocol/commands,*.c)
    C_FILES += $(wildcard $(CURRENT_DIR)/protocol/*.c)
    C_FILES += $(wildcard $(CURRENT_DIR)/protocol/serial/*.c)
endif
ifneq ($(filter remote,$(PARTS)),)
    C_FILES += $(wildcard $(CURRENT_DIR)/remote/*.c)
endif

ifdef ETH_PHY_TYPE
    ifneq ($(filter protocol,$(PARTS)),)
        C_FILES += $(wildcard $(CURRENT_DIR)/protocol/ethernet/*.c)
    endif
    C_FILES += $(wildcard $(CURRENT_DIR)/platform/ethernet/lwip/*.c)
endif
C_FILES += $(wildcard $(CURRENT_DIR)/platform/*.c)
C_FILES += $(wildcard $(CURRENT_DIR)/platform/ids/*.c)
C_FILES += $(wildcard $(CURRENT_DIR)/platform/led/*.c)
C_FILES += $(wildcard $(CURRENT_DIR)/platform/spiLinker/*.c)

C_FILES += $(wildcard $(CURRENT_DIR)/protocol/libusb/*.c)
C_FILES += $(call rwildcard,$(CURRENT_DIR)/target,*.c)


INCLUDE_DIRS += $(CURRENT_DIR)


# 3. Include makelist.mk from other dependencies
DEPENDENCIES :=


include $(DEPENDENCIES)

