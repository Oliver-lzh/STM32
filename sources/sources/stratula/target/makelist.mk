
# 1. Determine the current directory
CURRENT_DIR := $(call current_dir)


# 2. Add local C files, includes and flags

C_FILES += $(wildcard $(CURRENT_DIR)/*.c)
C_FILES += $(wildcard $(CURRENT_DIR)/board/*.c)


INCLUDE_DIRS += $(CURRENT_DIR)


# 3. Include makelist.mk from other dependencies
DEPENDENCIES :=


include $(DEPENDENCIES)
