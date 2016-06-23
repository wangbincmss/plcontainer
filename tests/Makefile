include ../Makefile

REGRESS_OPTS = --dbname=$(PL_TESTDB)
REGRESS = plcontainer_install plcontainer_schema plcontainer_populate plcontainer_function plcontainer_test_python plcontainer_test_anaconda plcontainer_test_r
PSQLDIR = $(bindir)

.PHONY: default
default: all

.PHONY: all
all: tests

.PHONY: tests
tests:
	$(top_builddir)/src/test/regress/pg_regress \
				--psqldir=$(PSQLDIR) $(REGRESS_OPTS) $(REGRESS)

.PHONY: submake
submake:
	$(MAKE) -C $(top_builddir)/src/test/regress pg_regress$(X)
