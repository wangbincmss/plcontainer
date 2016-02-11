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

.PHONY: containers
containers:
	docker build -f Dockerfile.R -t plc_r $(top_builddir)
	docker build -f Dockerfile.python -t plc_python $(top_builddir)

.PHONY: cleancontainers
cleancontainers:
	docker rmi -f plc_python
	docker rmi -f plc_r
