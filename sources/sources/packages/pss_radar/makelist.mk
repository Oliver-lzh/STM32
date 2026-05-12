

# 1. Determine the current directory
CURRENT_DIR := $(call current_dir)


# 2. Add local C files, includes and flags

C_FILES += $(call rwildcard,$(CURRENT_DIR)/processing,*.c)


INCLUDE_DIRS += $(CURRENT_DIR)/library


# 3. Include makelist.mk from other dependencies
DEPENDENCIES :=


include $(DEPENDENCIES)
