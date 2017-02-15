TOP=..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

#------------------------generic definitions----------------------#

CROSS_COMPILER_TARGET_ARCHS=

STATIC_BUILD = NO

DBD = opcUa.dbd

LIBRARY_HOST += opcUa
opcUa_SRCS = devOpcUa.c drvOpcUa.cpp devUaSubscription.cpp
INC += devOpcUa.h drvOpcUa.h
###

USR_SYS_LIBS += crypto
USR_SYS_LIBS += xml2

uastack_DIR=$(UASDK)/lib
USR_LIBS += uastack

USR_INCLUDES += -I$(UASDK)/include/uabase
USR_INCLUDES += -I$(UASDK)/include/uaclient
USR_INCLUDES += -I$(UASDK)/include/uapki
USR_INCLUDES += -I$(UASDK)/include/uastack
USR_INCLUDES += -I$(UASDK)/include/xmlparser
USR_INCLUDES += -I$(BOOST)

ifeq ($(OS), Windows_NT)
    EXTLIB_INSTALLS += $(INSTALL_LIB)/uastack.lib
else
    EXTLIB_INSTALLS += $(INSTALL_LIB)/libuastack.so
endif

opcUa_DIR = $(UASDK)/lib
opcUa_LIBS += uaclient
opcUa_LIBS += uapki
opcUa_LIBS += uabase
opcUa_LIBS += xmlparser

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

build: $(EXTLIB_INSTALLS)

$(INSTALL_LIB)/%.lib : $(UASDK)/lib/%.lib
	$(ECHO) "Installing $@, $<"
	$(INSTALL) -d -m 444 $< $(@D)

$(INSTALL_LIB)/lib%.a : $(UASDK)/lib/lib%.a
	$(ECHO) "Installing $@, $<"
	$(INSTALL) -d -m 444 $< $(@D)

$(INSTALL_LIB)/lib%.so : $(UASDK)/lib/lib%.so
	$(ECHO) "Installing $@, $<"
	$(INSTALL) -d -m 555 $< $(@D)


