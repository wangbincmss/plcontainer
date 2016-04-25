MODULE_big = plcontainer

# Directories
SRCDIR = ./src
MGMTDIR = ./management

# Files to build
FILES = $(shell find $(SRCDIR) -not -path "*client*" -type f -name "*.c")
OBJS = $(foreach FILE,$(FILES),$(subst .c,.o,$(FILE)))

PGXS := $(shell pg_config --pgxs)
include $(PGXS)

all: all-lib

install: all installdirs install-lib install-extra

installdirs: installdirs-lib
	$(MKDIR_P) '$(DESTDIR)$(bindir)'
	$(MKDIR_P) '$(DESTDIR)$(datadir)/plcontainer'

.PHONY: uninstall
uninstall: uninstall-lib
	rm -f '$(DESTDIR)$(bindir)/plcontainer-config'
	rm -f '$(DESTDIR)$(datadir)/plcontainer/'

.PHONY: install-extra
install-extra:
	$(INSTALL_PROGRAM) '$(MGMTDIR)/bin/plcontainer-config' '$(DESTDIR)$(bindir)/plcontainer-config'
	$(INSTALL_DATA) '$(MGMTDIR)/config/plcontainer_configuration.xml' '$(DESTDIR)$(datadir)/plcontainer/'
	$(INSTALL_DATA) '$(MGMTDIR)/sql/plcontainer_install.sql' '$(DESTDIR)$(datadir)/plcontainer/'

.PHONY: installcheck
installcheck:
	$(MAKE) -C tests

.PHONY: clients
clients:
	$(MAKE) -C $(SRCDIR)/pyclient
	$(MAKE) -C $(SRCDIR)/rclient

.PHONY: containers
containers: clients
	docker build -f dockerfiles/Dockerfile.R -t plc_r .
	docker build -f dockerfiles/Dockerfile.python -t plc_python .
	docker build -f dockerfiles/Dockerfile.R.shared -t plc_r_shared .
	docker build -f dockerfiles/Dockerfile.python.shared -t plc_python_shared .
	#docker build -f dockerfiles/Dockerfile.python.anaconda -t plc_anaconda .

.PHONY: cleancontainers
cleancontainers:
	docker rmi -f plc_python
	docker rmi -f plc_r
	docker rmi -f plc_python_shared
	docker rmi -f plc_r_shared
	#docker rmi -f plc_anaconda
	docker ps -a | grep -v CONTAINER | awk '{ print $$1}' | xargs -i docker rm {}
