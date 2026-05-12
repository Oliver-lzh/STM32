# =================================================================================================
# File:     mk_format.mk
# -------------------------------------------------------------------------------------------------
# Brief:    Creates a target format_all and format_check_all for files with FORMAT_EXTENSIONS within FORMAT_DIRS(_RECURSIVE)
#
# =================================================================================================

MAKE_BASE_DIR  := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

include $(MAKE_BASE_DIR)/mk_extensions.mk


ifeq (format,$(firstword $(MAKECMDGOALS)))
    PARAMS := $(filter-out format,$(MAKECMDGOALS))
    ifneq (,$(PARAMS))
        FORMAT_DIRS_RECURSIVE :=
        ifeq (,$(word 2,$(PARAMS)))
        # if only one parameter is specified, check if it is a file
            ifeq (,$(wildcard $(PARAMS)/.))
$(PARAMS): $(PARAMS)-format
                PARAMS :=
                FORMAT_NO_ECHO := 1
            endif
        endif
        FORMAT_DIRS := $(addsuffix /.,$(PARAMS))
# this is necessary to suppress make output on the fake target
.PHONY: $(PARAMS)
$(PARAMS): ;@:
    endif
endif


CLANG_FORMAT_APPLY_COMMAND = clang-format --style=file --verbose -i
CLANG_FORMAT_CHECK_COMMAND = clang-format --style=file --dry-run --Werror --verbose

CLANG_FORMAT_DIRS := $(FORMAT_DIRS)
CLANG_FORMAT_DIRS += $(foreach folder,$(FORMAT_DIRS_RECURSIVE),$(call rwildcard,$(folder),.))

CLANG_FORMAT_DIRS := $(dir $(CLANG_FORMAT_DIRS))
CLANG_FORMAT_DIRS := $(foreach folder,$(CLANG_FORMAT_DIRS),$(foreach ext,$(FORMAT_EXTENSIONS),$(if $(wildcard $(folder)$(ext)),$(folder)$(ext))))


format_all: $(addsuffix -format,$(CLANG_FORMAT_DIRS)) ;@:
ifndef FORMAT_NO_ECHO
	@echo Formatting of source and header files done.
endif
.PHONY: format_all

format_check_all:$(addsuffix -format_check,$(CLANG_FORMAT_DIRS)) ;@:
ifndef FORMAT_NO_ECHO
	@echo Format Checking of source and header files done.
endif
.PHONY: format_check_all

%-format:
	@$(CLANG_FORMAT_PATH)$(CLANG_FORMAT_APPLY_COMMAND) $*

%-format_check:
	@$(CLANG_FORMAT_PATH)$(CLANG_FORMAT_CHECK_COMMAND) $*
