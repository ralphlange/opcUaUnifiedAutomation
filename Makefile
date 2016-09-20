#Makefile at top of application tree
TOP = .
include $(TOP)/configure/CONFIG
DIRS := $(DIRS) $(filter-out $(DIRS), configure)
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *App))

RSYNC_DIST_VERSION_FILE=iocBoot/version

include $(TOP)/configure/RULES_TOP
