# Add the preprocessor definitions needed to set the version of the firmware in the version header file
# The VERSION variable must be set in SEMVER format before

VERSION_LIST := $(subst ., ,$(VERSION)) 0 0 # at least major and minor have to be defined

CPPFLAGS += -DFIRMWARE_VERSION_MAJOR=$(word 1,$(VERSION_LIST))
CPPFLAGS += -DFIRMWARE_VERSION_MINOR=$(word 2,$(VERSION_LIST))
CPPFLAGS += -DFIRMWARE_VERSION_PATCH=$(word 3,$(VERSION_LIST))
CPPFLAGS += -DFIRMWARE_VERSION_BUILD=$(word 4,$(VERSION_LIST))
