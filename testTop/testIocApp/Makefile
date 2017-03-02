TOP=..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

DB = testServer.db freeopcuaTEST.db

IOCS += OPCUAIOC

PROD_IOC = $(IOCS)
DBD = $(IOCS:%=%.dbd)

define RRDD
$(ioc)_SRCS += $(ioc)_registerRecordDeviceDriver.cpp SoftIocMain.cpp 
endef

$(foreach ioc,$(PROD_IOC),$(eval $(RRDD)))

ifeq ($(UASDK_DEPLOY_MODE),PROVIDED)
define UA_template
  $(1)_DIR = $(UASDK_DIR)
endef
$(foreach lib, $(UASDK_LIBS), $(eval $(call UA_template,$(lib))))
endif

INSTALL_BOOT =$(TOP)/iocBoot
STCMD_INSTALLS = $(IOCS:%=$(INSTALL_BOOT)/ioc%/st.cmd)

Standard_DBD += base.dbd
Standard_DBD += iocshGlobalCommands.dbd

#-----------------------------OPCUAIOC-----------------------------#

OPCUAIOC_DBD += $(Standard_DBD)
OPCUAIOC_DBD += opcUa.dbd

OPCUAIOC_LIBS += opcUa
OPCUAIOC_LIBS += $(UASDK_LIBS)
OPCUAIOC_LIBS += $(EPICS_BASE_IOC_LIBS)

OPCUAIOC_SYS_LIBS += xml2 crypto

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

build: $(STCMD_INSTALLS)

# install generated st.cmd files
$(STCMD_INSTALLS): $(INSTALL_BOOT)/ioc%/st.cmd: ../st.cmd.%
	$(ECHO) "Installing $@"
	@$(INSTALL) -d -m 755 $< $(@D)
	@$(MV) $(@D)/st.cmd.$* $@

