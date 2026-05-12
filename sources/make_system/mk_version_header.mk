# Read out the VERSION variable from the specified version header file

VERSION_FILE ?= $(REPO_DIR)/version.h
VERSION_TEXT := $(shell $(CAT) $(VERSION_FILE))

VERSION := $(word 3, $(VERSION_TEXT)).$(word 6, $(VERSION_TEXT)).$(word 9, $(VERSION_TEXT)).$(word 12, $(VERSION_TEXT))
