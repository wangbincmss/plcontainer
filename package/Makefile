all: gppkg

PGXS := $(shell pg_config --pgxs)
include $(PGXS)

OS=$(word 1,$(subst _, ,$(BLD_ARCH)))
ARCH=$(shell uname -p)

PLC_DIR=`cd .. && pwd`
PLC_VER=0.1
PLC_REL=0
PLC_GPPKG_VER=$(PLC_VER).$(PLC_REL)_gpdb4.3orca
PLC_RPM_FLAGS="--define 'plc_dir $(PLC_DIR)' --define 'plc_ver $(PLC_VER)' --define 'plc_rel $(PLC_REL)'"
PLC_RPM=plcontainer-$(PLC_VER)-$(PLC_REL).$(ARCH).rpm
PLC_GPPKG=plcontainer-$(PLC_GPPKG_VER)-$(OS)-$(ARCH).gppkg

TARGET_GPPKG=$(PLC_GPPKG)
EXTRA_CLEAN+=$(R_RPM) $(PLR_RPM) $(PLR_GPPKG)

#
# Generic rules to build gppkgs included here
#
include gppkg.mk

.PHONY: gppkg
gppkg:
	$(MAKE) $(PLC_RPM) RPM_FLAGS=$(PLC_RPM_FLAGS)
	$(MAKE) $(PLC_GPPKG) MAIN_RPM=$(PLC_RPM)

.PHONY: cleanall
cleanall: clean
	rm -rf BUILDROOT
	rm -rf SOURCES
	rm -rf SRPMS
	rm -rf $(PLC_RPM)
	rm -rf $(PLC_GPPKG)
