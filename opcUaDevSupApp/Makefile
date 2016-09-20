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

###

USR_SYS_LIBS += crypto
USR_SYS_LIBS += xml2

USR_LIBS += uastack
uastack_DIR=$(UASDK)/lib


USR_INCLUDES += -I$(UASDK)/include/uabase
USR_INCLUDES += -I$(UASDK)/include/uaclient
USR_INCLUDES += -I$(UASDK)/include/uamodels
USR_INCLUDES += -I$(UASDK)/include/uapki
USR_INCLUDES += -I$(UASDK)/include/uaserver
USR_INCLUDES += -I$(UASDK)/include/uastack
USR_INCLUDES += -I$(UASDK)/include/xmlparser

EXTLIB_INSTALLS += $(INSTALL_LIB)/libuastack.so
EXTLIB_INSTALLS += $(INSTALL_LIB)/libuamodels.a
EXTLIB_INSTALLS += $(INSTALL_LIB)/libuamodule.a
EXTLIB_INSTALLS += $(INSTALL_LIB)/libcoremodule.a
EXTLIB_INSTALLS += $(INSTALL_LIB)/libuaclient.a
EXTLIB_INSTALLS += $(INSTALL_LIB)/libuapki.a
EXTLIB_INSTALLS += $(INSTALL_LIB)/libuabase.a
EXTLIB_INSTALLS += $(INSTALL_LIB)/libxmlparser.a

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

build: $(EXTLIB_INSTALLS)

$(INSTALL_LIB)/lib%.a : $(UASDK)/lib/lib%.a
	$(ECHO) "Installing $@, $<"
	$(INSTALL) -d -m 444 $< $(@D)

$(INSTALL_LIB)/lib%.so : $(UASDK)/lib/lib%.so
	$(ECHO) "Installing $@, $<"
	$(INSTALL) -d -m 555 $< $(@D)
