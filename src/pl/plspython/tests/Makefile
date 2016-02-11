.PHONY: default
default: all

top_builddir = ../../../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/src/Makefile.shlib

REGRESS_OPTS = --dbname=$(PL_TESTDB) --load-language=plspython
REGRESS = plspython_schema plspython_populate plspython_function plspython_test
PSQLDIR = $(bindir)

.PHONY: all
all: tests

.PHONY: tests
tests: submake
	$(top_builddir)/src/test/regress/pg_regress \
				--psqldir=$(PSQLDIR) $(REGRESS_OPTS) $(REGRESS)

.PHONY: submake
submake:
	$(MAKE) -C $(top_builddir)/src/test/regress pg_regress$(X)

