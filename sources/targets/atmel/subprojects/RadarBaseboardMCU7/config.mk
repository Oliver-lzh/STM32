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

CONTRIB_PATH := ../../target_platform/contrib

# Path to top level ASF directory relative to this project directory.
PRJ_PATH = .

BOARD = USER_BOARD

# Target CPU architecture: cortex-m3, cortex-m4
ARCH = cortex-m7

# Target part: none, sam3n4 or sam4l4aa
PART = SAMS70Q21B

# Application target name. Given with suffix .a for library and .elf for a
# standalone application.
TARGET_FLASH = $(BUILD_DIR)/RadarBaseboardMCU7.elf
TARGET_SRAM = $(BUILD_DIR)/RadarBaseboardMCU7.elf

# List of C source files.
CSRCS = $(C_FILES)

# List of assembler source files.
ASSRCS = 

# List of include paths.
INC_PATH = $(INCLUDE_DIRS)

# Additional search paths for libraries.
LIB_PATH = $(LIB_DIRS) \
       thirdparty/CMSIS/Lib/GCC

# List of libraries to use during linking.
LIBS +=  \
       arm_cortexM7lfsp_math_softfp                       \
       m

# Path relative to top level directory pointing to a linker script.
LINKER_SCRIPT_FLASH = $(CONTRIB_PATH)/ASF/sam/utils/linker_scripts/sams70/sams70q21/gcc/flash.ld
LINKER_SCRIPT_SRAM  = $(CONTRIB_PATH)/ASF/sam/utils/linker_scripts/sams70/sams70q21/gcc/sram.ld

# Path relative to top level directory pointing to a linker script.
DEBUG_SCRIPT_FLASH = $(CONTRIB_PATH)/debug_scripts/gcc/flash.gdb
DEBUG_SCRIPT_SRAM  = $(CONTRIB_PATH)/debug_scripts/gcc/sram.gdb

# Project type parameter: all, sram or flash
PROJECT_TYPE        = flash

# Additional options for debugging. By default the common Makefile.in will
# add -g3.
#DBGFLAGS =

# Application optimization used during compilation and linking:
# -O0, -O1, -O2, -O3 or -Os
#OPTIMIZATION = -O1

ifeq ($(BUILD_TYPE),debug)
    BUILD_DEBUG_LEVEL = 3
    BUILD_OPTIMIZATION = 1
endif

ifeq ($(BUILD_TYPE),release)
    BUILD_DEBUG_LEVEL = 0
    BUILD_OPTIMIZATION = s
endif

# Extra flags to use when archiving.
ARFLAGS = 

# Extra flags to use when assembling.
ASFLAGS =  \
       -mfloat-abi=softfp                                 \
       -mfpu=fpv5-sp-d16

# Extra flags to use when compiling.
CFLAGS +=  \
       -mfloat-abi=softfp                                 \
       -mfpu=fpv5-sp-d16                                  \
       -MMD

# Extra flags to use when preprocessing.
#
# Preprocessor symbol definitions
#   To add a definition use the format "-D name[=definition]".
#   To cancel a definition use the format "-U name".
#
# The most relevant symbols to define for the preprocessor are:
#   BOARD      Target board in use, see boards/board.h for a list.
#   EXT_BOARD  Optional extension board in use, see boards/board.h for a list.
CPPFLAGS += \
       -D ARM_MATH_CM7=true                               \
       -D __FPU_PRESENT=1                                  \
       -D BOARD=$(BOARD)                                  \
       -D printf=iprintf                                  \
       -D scanf=iscanf

# Extra flags to use when linking
LDFLAGS += \
    -L$(CONTRIB_PATH)/ASF/thirdparty/CMSIS/Lib/GCC

# Pre- and post-build commands
PREBUILD_CMD = 
POSTBUILD_CMD = 