NAME = plcontainer
FILES = $(shell find . -not -path "*client*" -type f -name "*.c")
OBJS = $(foreach FILE,$(FILES),$(subst .c,.o,$(FILE)))

top_builddir = ../../..
include $(top_builddir)/src/Makefile.global
include $(top_builddir)/src/Makefile.shlib

MGMTDIR = ./management

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
	$(INSTALL_PROGRAM) '$(MGMTDIR)/plcontainer-config' '$(DESTDIR)$(bindir)/plcontainer-config'
	$(INSTALL_DATA) '$(MGMTDIR)/plcontainer_configuration.xml' '$(DESTDIR)$(datadir)/plcontainer/'

.PHONY: installcheck
installcheck:
	$(MAKE) -C tests

.PHONY: clean
clean:
	rm -f *.o
	rm -f common/*.o
	rm -f common/messages/*.o
	rm -f plcontainer.so

.PHONY: clients
clients:
	$(MAKE) -C pyclient
	$(MAKE) -C rclient

.PHONY: containers
containers: clients
	docker build -f dockerfiles/Dockerfile.R -t plc_r .
	docker build -f dockerfiles/Dockerfile.python -t plc_python .
	docker build -f dockerfiles/Dockerfile.R.shared -t plc_r_shared .
	docker build -f dockerfiles/Dockerfile.python.shared -t plc_python_shared .

.PHONY: containers6
containers6: clients
	docker build -f dockerfiles/Dockerfile.R6 -t plc_r .
	docker build -f dockerfiles/Dockerfile.python6 -t plc_python .
	docker build -f dockerfiles/Dockerfile.R.shared6 -t plc_r_shared .
	docker build -f dockerfiles/Dockerfile.python.shared6 -t plc_python_shared .

.PHONY: cleancontainers
cleancontainers:
	docker rmi -f plc_python
	docker rmi -f plc_r
	docker rmi -f plc_python_shared
	docker rmi -f plc_r_shared
	docker ps -a | grep -v CONTAINER | awk '{ print $$1}' | xargs -i docker rm {}
