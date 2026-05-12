# =================================================================================================
# File:     build/mk_post.mk
# -------------------------------------------------------------------------------------------------
# Brief:    Global targets for building the subprojects.
#
# =================================================================================================

# include project specific dependencies
include $(DEPENDENCIES)


CPP_FILES += $(filter %.cpp, $(SOURCE_FILES))
C_FILES += $(filter %.c, $(SOURCE_FILES))
ASM_FILES += $(filter %.S, $(SOURCE_FILES))


OBJS =  $(CPP_FILES:.cpp=.o) $(C_FILES:.c=.o) $(ASM_FILES:.S=.o)

OBJS := $(OBJS:$(CUSTOM_REPO_DIR)/%=%)
OBJS := $(OBJS:$(REPO_DIR)/%=%)
OBJS := $(subst ../,,$(OBJS))

OBJS := $(addprefix $(BUILD_DIR)/, $(OBJS))
OBJS_COUNT := $(words $(OBJS))


VPATH += $(dir $(TARGET_PLATFORM_DIR))


include $(MAKE_BASE_DIR)/mk_toolchain.mk

clean:
	@$(RMDIR) $(BUILD_DIR)

$(BUILD_DIR):
	@$(MKDIR) $@

$(OBJS): | $(BUILD_DIR) BUILD_PARSER

