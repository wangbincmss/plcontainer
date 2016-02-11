subdir = src/pl/plpython
top_builddir = ../../..
include $(top_builddir)/src/Makefile.global

# On some platforms we can only build PL/Python if libpython is a
# shared library.  Since there is no official way to determine this
# (at least not in pre-2.3 Python), we see if there is a file that is
# named like a shared library.
ifneq (,$(wildcard $(python_libdir)/libpython*$(DLSUFFIX)*))
shared_libpython = yes
endif

# Darwin (OS X) has its own ideas about how to do this.
ifeq ($(PORTNAME), darwin)
shared_libpython = yes
# Replaced to allow use of Greenplum-provided Python
#override python_libspec = -framework Python
#override python_additional_libs =
# sys.prefix reflects the runtime location of the python tree;
# the GP install has the dylib in the lib directory of that tree.
override python_libspec = -L$(shell ${PYTHON} -c "import sys, os; print os.path.join(sys.prefix, 'lib')") -lpython${python_version}
endif

override python_libspec := -L${python_configdir}/../.. ${python_libspec}

rpathdir = $(python_libdir):$(INSTLOC)/ext/python/lib

NAME = plspython
FILES = $(shell find . -not -path "*client*" -type f -name "*.c")
OBJS = $(foreach FILE,$(FILES),$(subst .c,.o,$(FILE)))

# If we don't have a shared library and the platform doesn't allow it
# to work without, we have to skip it.
ifneq (,$(findstring yes, $(shared_libpython)$(allow_nonpic_in_shlib)))

# do not fail build due to warnings in this code
#override CPPFLAGS := -Wno-error -I$(srcdir) $(python_includespec) $(CPPFLAGS) -DPLPYTHON_SHOW_DEBUG_INFO
override CPPFLAGS := -Wno-error -I$(srcdir) $(python_includespec) $(CPPFLAGS)

SHLIB_LINK = $(python_libspec) $(python_additional_libs) $(filter -lintl,$(LIBS))

include $(top_srcdir)/src/Makefile.shlib

all: all-lib

#install: all containers installdirs install-lib
install: all installdirs install-lib

installdirs: installdirs-lib

uninstall: uninstall-lib

.PHONY: containers
containers:
	docker build -f Dockerfile.R -t plr $(top_builddir)
	docker build -f Dockerfile.python -t plspython $(top_builddir)

.PHONY: installcheck
installcheck:
	$(MAKE) -C tests

.PHONY: clean
clean:
	rm -f *.o
	rm -f common/*.o
	rm -f common/messages/*.o
	rm -f plspython.so
	#docker rmi -f plspython
	#docker rmi -f plr

else # can't build

all:
	@echo ""; \
	 echo "*** Cannot build PL/SPython because libpython is not a shared library." ; \
	 echo "*** You might have to rebuild your Python installation.  Refer to"; \
	 echo "*** the documentation for details."; \
	 echo ""

endif # can't build
