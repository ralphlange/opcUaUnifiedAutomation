TOP=..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

#------------------------generic definitions----------------------#

CROSS_COMPILER_TARGET_ARCHS=

STATIC_BUILD = NO

DB = OPCUA_RECORD.db freeopcuaTEST.db

IOCS+= OPCUAIOC

PROD_IOC = $(IOCS)
DBD = $(IOCS:%=%.dbd)

define RRDD
$(ioc)_SRCS += $(ioc)_registerRecordDeviceDriver.cpp SoftIocMain.cpp 
endef

$(foreach ioc,$(PROD_IOC),$(eval $(RRDD)))

INSTALL_BOOT =$(TOP)/iocBoot
STCMD_INSTALLS = $(IOCS:%=$(INSTALL_BOOT)/ioc%/st.cmd)

Standard_DBD += base.dbd
Standard_DBD += iocshGlobalCommands.dbd

#-----------------------------OPCUAIOC-----------------------------#

OPCUAIOC_DBD += $(Standard_DBD)
OPCUAIOC_DBD += opcUa.dbd

OPCUAIOC_LIBS += $(EPICS_BASE_IOC_LIBS)
OPCUAIOC_SYS_LIBS += crypto
OPCUAIOC_SYS_LIBS += xml2
OPCUAIOC_LIBS += opcUa
OPCUAIOC_LIBS += uamodels
OPCUAIOC_LIBS += uamodule
OPCUAIOC_LIBS += coremodule
OPCUAIOC_LIBS += uaclient
OPCUAIOC_LIBS += uapki
OPCUAIOC_LIBS += uabase
OPCUAIOC_LIBS += uastack
OPCUAIOC_LIBS += xmlparser


include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

build: $(STCMD_INSTALLS)

# install generated st.cmd files
$(STCMD_INSTALLS): $(INSTALL_BOOT)/ioc%/st.cmd: ../st.cmd.%
	$(ECHO) "Installing $@"
	@$(INSTALL) -d -m 755 $< $(@D)
	@$(MV) $(@D)/st.cmd.$* $@

