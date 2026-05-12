

# 1. Determine the current directory
CURRENT_DIR := $(call current_dir)


# 2. Add local C files, includes and flags

#C_FILES += 


#INCLUDE_DIRS += 


# 3. Include makelist.mk from other dependencies
DEPENDENCIES :=

DEPENDENCIES += $(wildcard $(CURRENT_DIR)/*/makelist.mk)

include $(DEPENDENCIES)
