
# 1. Determine the current directory
CURRENT_DIR := $(call current_dir)


# 2. Add local C files, includes and flags

C_FILES += $(wildcard $(CURRENT_DIR)/*.c)


INCLUDE_DIRS += $(CURRENT_DIR)


# 3. Include makelist.mk from other dependencies
DEPENDENCIES :=

DEPENDENCIES += $(CURRENT_DIR)/stratula/makelist.mk
DEPENDENCIES += $(CURRENT_DIR)/packages/makelist.mk

include $(DEPENDENCIES)
