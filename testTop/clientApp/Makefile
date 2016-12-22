TOP=..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

#------------------------generic definitions----------------------#

CROSS_COMPILER_TARGET_ARCHS=

STATIC_BUILD = NO

PROD = opcUaClient
PROD_SRCS = clientMain.cpp

opcUaClient_DIR = $(UASDK)/lib
opcUaClient_LIBS += opcUa
opcUaClient_LIBS += $(EPICS_BASE_IOC_LIBS)
opcUaClient_LIBS += uastack
opcUaClient_LIBS += uaclient
opcUaClient_LIBS += uapki
opcUaClient_LIBS += uabase
opcUaClient_LIBS += xmlparser

USR_INCLUDES += -I$(UASDK)/include/uabase
USR_INCLUDES += -I$(UASDK)/include/uaclient
USR_INCLUDES += -I$(UASDK)/include/uapki
USR_INCLUDES += -I$(UASDK)/include/uastack
USR_INCLUDES += -I$(UASDK)/include/xmlparser

# set rpath than the libraries can be found at your install location
# check path with: 'readelf -d ../bin/linux-x86_64/opcUcClient'
#
#USR_LDFLAGS += '-Wl,-rpath,/home/kuner/tmp/testUa'

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
