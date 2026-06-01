# List of available make goals:
#
# all                     Default target, builds the project
# clean                   Clean up the project
# rebuild                 Rebuild the project
#
#
# doc                     Build the documentation
# cleandoc                Clean up the documentation
# rebuilddoc              Rebuild the documentation
#
# Copyright (c) 2011 Atmel Corporation. All rights reserved.
#
# \asf_license_start
#
# \page License
#
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
#
# \asf_license_stop
#

include ../subprojects.mk


# the USB_CLASS_TYPE must correspond to the COMMUNICATION define in BoardDefinition.h
USB_CLASS_TYPE = cdc
#USB_CLASS_TYPE = vendor


include $(MAKE_SYSTEM_PATH)/build/mk_pre.mk
include ../../target_platform/makelist.mk

C_FILES := $(subst ../,,$(C_FILES))


C_FILES += $(filter-out copy_fft.c,$(wildcard *.c))

INCLUDE_DIRS += .


all: | BUILD_PARSER
BUILD_PARSER:
	@echo **** Build Parser Output:
	@echo $(CC) $(c_flags) dummy.c
	@echo **** Starting Build ...


ifneq ($(findstring Windows,$(OS)),)
    OS :=Windows
    ARM_GCC_PATH ?= C:\"Program Files (x86)"\Atmel\Studio\7.0\toolchain\arm\arm-gnu-toolchain\bin\\# last backslash must be escaped and not followed by any whitespace
endif
CROSS ?= $(ARM_GCC_PATH)arm-none-eabi-


# Include the common Makefile, which will also include the project specific
# config.mk file.
MAKEFILE_PATH = ../../target_platform/contrib/ASF/sam/utils/make/Makefile.sam.in
include $(MAKEFILE_PATH)


include ../extended_recipes.mk

