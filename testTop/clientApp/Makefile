TOP=..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

#------------------------generic definitions----------------------#

CROSS_COMPILER_TARGET_ARCHS=

STATIC_BUILD = NO

UASDK = /home/pc/kuner/ctl/OPC_UA/uasdkcppclient-src-debian7.7-x86_64-gcc4.7.2-v1.5.3-346/sdk

PROD = opcUcClient
PROD_SRCS = clientMain.cpp

opcUcClient_LIBS += $(EPICS_BASE_IOC_LIBS)
opcUcClient_LIBS += opcUa
opcUcClient_LIBS += uastack

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
