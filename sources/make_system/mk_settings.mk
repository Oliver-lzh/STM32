# =================================================================================================
# File:     mk_settings.mk
# -------------------------------------------------------------------------------------------------
# Brief:    Global defines used for all makesystem configuration.
#
# =================================================================================================

## Global defines:
REPO_DIR        ?= ../..
CUSTOM_REPO_DIR ?= ../../..
LOCAL_DIR        = .

COMMON_SRC_DIR  ?= $(REPO_DIR)/sources

PACKAGE_DIR     ?= $(REPO_DIR)/packages
TEST_DIR        ?= $(REPO_DIR)/tests
TEMPLATE_DIR    ?= $(REPO_DIR)/make_system/templates

BUILD_TYPE      ?= DEBUG
BUILD_TYPE      := $(call lowercase,$(strip $(BUILD_TYPE)))
BUILD_DIR       ?= build-$(BUILD_TYPE)
BUILD_DIR       := $(strip $(BUILD_DIR))

C_STANDARD      ?= 99
C_STANDARD      := $(strip $(C_STANDARD))
CXX_STANDARD    ?= 11
CXX_STANDARD    := $(strip $(CXX_STANDARD))


# Initialization
.DEFAULT_GOAL   ?= all

IMAGE           ?= target
TARGET          ?= $(BUILD_DIR)/$(IMAGE)
TARGET_HEX      ?= $(TARGET).hex


___TARGET_SEPARATOR_LINE___ := --------------------------------------------
