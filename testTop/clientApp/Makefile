TOP=..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

PROD = opcUaClient
PROD_SRCS = clientMain.cpp

PROD_LIBS += opcUa
PROD_LIBS += $(UASDK_LIBS)
PROD_LIBS += $(EPICS_BASE_IOC_LIBS)

PROD_SYS_LIBS += xml2 crypto

ifeq ($(UASDK_DEPLOY_MODE),PROVIDED)
define UA_template
  $(1)_DIR = $(UASDK_DIR)
endef
$(foreach lib, $(UASDK_LIBS), $(eval $(call UA_template,$(lib))))
endif

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
