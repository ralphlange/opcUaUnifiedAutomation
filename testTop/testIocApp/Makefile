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
OPCUAIOC_LIBS += opcUa
OPCUAIOC_LIBS += uastack

# set rpath than the libraries can be found at your install location
# check path with: 'readelf -d ../bin/linux-x86_64/OPCUAIOC'
#
USR_LDFLAGS += '-Wl,-rpath,/home/kuner/tmp/testUa'

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

build: $(STCMD_INSTALLS)

# install generated st.cmd files
$(STCMD_INSTALLS): $(INSTALL_BOOT)/ioc%/st.cmd: ../st.cmd.%
	$(ECHO) "Installing $@"
	@$(INSTALL) -d -m 755 $< $(@D)
	@$(MV) $(@D)/st.cmd.$* $@

