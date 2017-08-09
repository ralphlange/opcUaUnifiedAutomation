TOP=..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

PROD = opcUaClient
PROD_SRCS = clientMain.cpp

PROD_LIBS += opcUa
PROD_LIBS += $(UASDK_LIBS)
PROD_LIBS += $(EPICS_BASE_IOC_LIBS)
PROD_SYS_LIBS += boost_regex

ifeq ($(UASDK_USE_XMLPARSER),YES)
PROD_SYS_LIBS_Linux += xml2
PROD_SYS_LIBS_WIN32 += libxml2 
USR_LDFLAGS_WIN32 += /LIBPATH:$(UASDK)/third-party/win32/vs2010sp1/libxml2/out32dll
endif

ifeq ($(UASDK_USE_CRYPTO),YES)
PROD_SYS_LIBS_Linux += crypto
PROD_SYS_LIBS_WIN32 += crypt32 ssleay32 libeay32
USR_LDFLAGS_WIN32 += /LIBPATH:$(UASDK)/third-party/win32/vs2010sp1/openssl/out32dll 
endif

ifeq ($(UASDK_DEPLOY_MODE),PROVIDED)
define UA_template
  $(1)_DIR = $(UASDK_DIR)
endef
$(foreach lib, $(UASDK_LIBS), $(eval $(call UA_template,$(lib))))
endif

PROD_SYS_LIBS_WIN32 += ws2_32 advapi32 user32 ole32 oleaut32 strmiids

USR_INCLUDES_WIN32 += -I$(GETOPT)/

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
