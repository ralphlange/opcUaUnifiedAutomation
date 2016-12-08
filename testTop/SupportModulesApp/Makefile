TOP=..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

MODULE_PATHS = $(SHRLIB_SEARCH_DIRS)
MODULE_LIBNAMES = $(notdir $(foreach dir,$(MODULE_PATHS),$(wildcard $(dir)/lib*.a)))
MODULE_NAMES = $(MODULE_LIBNAMES:$(LIB_PREFIX)%.a=%)

## vxWorks

MODULE_NMS = $(MODULE_NAMES:%=%.nm)
MODULES_vxWorks = $(addsuffix .munch,$(MODULE_NAMES))

## RTEMS

# ignore all GESYS libs
MODULES_RTEMS = $(addsuffix .obj,$(filter-out $(GESYS_LIBS:-l%=%),$(MODULE_NAMES)))

$(foreach os,vxWorks RTEMS,$(eval BIN_INSTALLS_$(os) = $(MODULES_$(os))))

# find the kernel...
RTEMS_KERNEL = $(wildcard $(RTEMS-BUILD-TOOL)/kernel/rtems-$(subst RTEMS-,,$(T_A))*)
# ...and install it
BIN_INSTALLS_RTEMS += rtems.bin

## Linux

MODULES_Linux = $(foreach dir,$(SHRLIB_SEARCH_DIRS),$(wildcard $(dir)/lib*.so*))
BIN_INSTALLS_Linux = $(MODULES_Linux)

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

vpath $(LIB_PREFIX)%.a $(MODULE_PATHS)

# need a static pattern rule here to effectively override the base rule
# which declares a dependency on %$(EXE)
$(MODULE_NMS): %.nm: %.obj
	$(NM) $< > $@

%.munch: %_ctdt.o %.obj
	$(MUNCH_CMD)

%.obj: $(LIB_PREFIX)%.a
	$(LD) --whole-archive -o $@ -l$* -L$(<D)

rtems.bin: $(RTEMS_KERNEL)
	$(CP) $< $@
