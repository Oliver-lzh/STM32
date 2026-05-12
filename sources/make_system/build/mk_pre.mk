# =================================================================================================
# File:     build/mk_pre.mk
# -------------------------------------------------------------------------------------------------
# Brief:    Variables and defines used for building the subprojects
#
# =================================================================================================

MAKE_BASE_DIR  := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))


include $(MAKE_BASE_DIR)/../mk_extensions.mk
include $(MAKE_BASE_DIR)/../mk_settings.mk


.DEFAULT_GOAL  := all
VPATH          += $(REPO_DIR)
VPATH          += $(CUSTOM_REPO_DIR)

# Force use of simply-expanded variables - required for hierarchical depdenency files to work
C_FILES        :=$(C_FILES)
INCLUDE_DIRS   :=$(INCLUDE_DIRS)
DEFINES        :=$(DEFINES)
CPPFLAGS       :=$(CPPFLAGS)
CFLAGS         :=$(CFLAGS)
CXXFLAGS       :=$(CXXFLAGS)
LIB_DIRS       :=$(LIB_DIRS)
LIBS           :=$(LIBS)
LDFLAGS        :=$(LDFLAGS)


include $(COMMON_SRC_DIR)/makelist.mk

-include $(CUSTOM_REPO_DIR)/makelist.mk
ifdef CUSTOM_SRC_DIR
    $(info **** makelist.mk from custom repository included!)
endif


# clear variable to be re-used for project specific dependencies
DEPENDENCIES :=


ifdef EXTENDED_VERSION
    CPPFLAGS += -DEXTENDED_VERSION=\"$(EXTENDED_VERSION)\"
endif
