
# 1. Determine the current directory
CURRENT_DIR := $(call current_dir)


# 2. Add local C files, includes and flags

#C_FILES += 


#INCLUDE_DIRS += 


# 3. Include makelist.mk from other dependencies
DEPENDENCIES :=

DEPENDENCIES += $(CURRENT_DIR)/library/makelist.mk
DEPENDENCIES += $(CURRENT_DIR)/target/makelist.mk
DEPENDENCIES += $(wildcard $(CURRENT_DIR)/contrib/makelist.mk)

include $(DEPENDENCIES)
