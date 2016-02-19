NAME = plcontainer
FILES = $(shell find . -not -path "*client*" -type f -name "*.c")
OBJS = $(foreach FILE,$(FILES),$(subst .c,.o,$(FILE)))

top_builddir = ../../..
include $(top_builddir)/src/Makefile.global
include $(top_builddir)/src/Makefile.shlib

all: all-lib

install: all installdirs install-lib

installdirs: installdirs-lib

uninstall: uninstall-lib

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
	docker build -f Dockerfile.R -t plc_r .
	docker build -f Dockerfile.python -t plc_python .

.PHONY: cleancontainers
cleancontainers:
	docker rmi -f plc_python
	#docker rmi -f plc_r
	docker ps -a | grep -v CONTAINER | awk '{ print \$1}' | xargs -i docker rm {}
