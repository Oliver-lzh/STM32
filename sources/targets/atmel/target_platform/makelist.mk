# Copyright (C) 2021 Infineon Technologies AG
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#   this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. Neither the name of the copyright holder nor the names of its
#    contributors may be used to endorse or promote products derived from
#    this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# 1. Determine the current directory
CURRENT_DIR := $(call current_dir)


# 2. Add local C files, includes and flags

C_FILES += $(call rwildcard,$(CURRENT_DIR)/contrib/ASF/common/services/clock,*.c)
C_FILES += $(call rwildcard,$(CURRENT_DIR)/contrib/ASF/common/services/ioport,*.c)
C_FILES += $(call rwildcard,$(CURRENT_DIR)/contrib/ASF/common/services/serial,*.c)
C_FILES += $(call rwildcard,$(CURRENT_DIR)/contrib/ASF/common/services/sleepmgr,*.c)
C_FILES += $(call rwildcard,$(CURRENT_DIR)/contrib/ASF/common/services/usb/udc,*.c)
C_FILES += $(call rwildcard,$(CURRENT_DIR)/contrib/ASF/common/services/usb/class/$(USB_CLASS_TYPE),*.c)
C_FILES += $(call rwildcard,$(CURRENT_DIR)/contrib/ASF/common/utils,*.c)
C_FILES += $(call rwildcard,$(CURRENT_DIR)/contrib/ASF/sam/drivers,*.c)
C_FILES += $(call rwildcard,$(CURRENT_DIR)/contrib/ASF/sam/services,*.c)
C_FILES += $(call rwildcard,$(CURRENT_DIR)/contrib/ASF/sam/utils,*.c)

C_FILES += $(wildcard $(CURRENT_DIR)/impl/*.c)
C_FILES += $(wildcard $(CURRENT_DIR)/impl/custom/*.c)
C_FILES += $(wildcard $(CURRENT_DIR)/impl/ids/*.c)
C_FILES += $(wildcard $(CURRENT_DIR)/impl/peripherals/*.c)

ifeq ($(USB_CLASS_TYPE),cdc)
    C_FILES += $(wildcard $(CURRENT_DIR)/impl/serial/*.c)
else
    C_FILES += $(wildcard $(CURRENT_DIR)/impl/libusb/*.c)
endif


INCLUDE_DIRS += $(CURRENT_DIR)
INCLUDE_DIRS += $(CURRENT_DIR)/contrib
INCLUDE_DIRS += $(call rwildcard,$(CURRENT_DIR)/contrib/ASF/sam/drivers,.)
INCLUDE_DIRS += \
$(CURRENT_DIR)/contrib/config \
$(CURRENT_DIR)/contrib/config/$(USB_CLASS_TYPE) \
$(CURRENT_DIR)/contrib/ASF/common/boards \
$(CURRENT_DIR)/contrib/ASF/common/boards/user_board \
$(CURRENT_DIR)/contrib/ASF/common/services/clock \
$(CURRENT_DIR)/contrib/ASF/common/services/ioport \
$(CURRENT_DIR)/contrib/ASF/common/services/serial \
$(CURRENT_DIR)/contrib/ASF/common/services/serial/sam_uart \
$(CURRENT_DIR)/contrib/ASF/common/services/sleepmgr \
$(CURRENT_DIR)/contrib/ASF/common/services/usb \
$(CURRENT_DIR)/contrib/ASF/common/services/usb/class/$(USB_CLASS_TYPE) \
$(CURRENT_DIR)/contrib/ASF/common/services/usb/class/$(USB_CLASS_TYPE)/device \
$(CURRENT_DIR)/contrib/ASF/common/services/usb/udc \
$(CURRENT_DIR)/contrib/ASF/common/utils \
$(CURRENT_DIR)/contrib/ASF/common/utils/stdio/stdio_serial \
$(CURRENT_DIR)/contrib/ASF/sam/services/flash_efc \
$(CURRENT_DIR)/contrib/ASF/sam/utils \
$(CURRENT_DIR)/contrib/ASF/sam/utils/cmsis/sams70/include \
$(CURRENT_DIR)/contrib/ASF/sam/utils/cmsis/sams70/source/templates \
$(CURRENT_DIR)/contrib/ASF/sam/utils/fpu \
$(CURRENT_DIR)/contrib/ASF/sam/utils/header_files \
$(CURRENT_DIR)/contrib/ASF/sam/utils/preprocessor \
$(CURRENT_DIR)/contrib/ASF/thirdparty/CMSIS/Include \
$(CURRENT_DIR)/contrib/ASF/thirdparty/CMSIS/Lib/GCC


# 3. Include makelist.mk from other dependencies
DEPENDENCIES :=


include $(DEPENDENCIES)
