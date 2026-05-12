
ifndef PLATFORM
   $(error "PLATFORM is undefined!")
endif
PLATFORM    :=$(strip $(PLATFORM))

ifndef MCPU
   $(error "MCPU is undefined!")
endif
MCPU        :=$(strip $(MCPU))

ifndef TOOLCHAIN
    $(error "TOOLCHAIN is undefined!")
endif
TOOLCHAIN   :=$(strip $(TOOLCHAIN))

BUILD_TYPE  :=$(strip $(BUILD_TYPE))
IMAGE       :=$(strip $(IMAGE))

$(info **** TOOLCHAIN: $(TOOLCHAIN))
$(info **** BUILD_TYPE: $(BUILD_TYPE))


CPPFLAGS += -DTARGET_PLATFORM_$(PLATFORM) -DTARGET_MCPU_$(MCPU)

include $(MAKE_BASE_DIR)/$(call lowercase,$(PLATFORM))/mk_tools.mk
include $(MAKE_BASE_DIR)/$(call lowercase,$(PLATFORM))/mk_compiler.mk
