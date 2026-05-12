# =================================================================================================
# File:     mk_tools.mk
# -------------------------------------------------------------------------------------------------
# Brief:    Global tools needed for the makesystem itself
#
# =================================================================================================

OSTYPE :=$(shell echo $$OSTYPE)
ifeq ($(OSTYPE),$$OSTYPE)
    OSTYPE :=windows
endif

#$(info OS: "$(OS)" - OSTYPE: "$(OSTYPE)")

CD              := cd
ECHO            := echo

ifeq ($(OSTYPE),windows)
    RM          := "$(MAKE_BASE_DIR)/../tools/win/rm.bat"
    CP          := "$(MAKE_BASE_DIR)/../tools/win/cp.bat"
    MV          := move /Y
    MKDIR       := "$(MAKE_BASE_DIR)/../tools/win/mkdir.bat"
    RMDIR       := "$(MAKE_BASE_DIR)/../tools/win/rmdir.bat"
    CAT         := "$(MAKE_BASE_DIR)/../tools/win/cat.bat"

    SED         := sed

    PROGRESS    := "$(MAKE_BASE_DIR)/../tools/win/progress.bat"
else
    RM          := rm -f
    CP          := cp -f
    MV          := mv
    MKDIR       := mkdir -p
    RMDIR       := rm -Rf
    CAT         := cat

    SED         := sed
    GREP        := grep

    PROGRESS    := "$(MAKE_BASE_DIR)/../tools/linux/progress.sh"
endif
