include ../Makefile

REGRESS_OPTS = --dbname=$(PL_TESTDB)
REGRESS_GPDB5 = plcontainer_install plcontainer_schema plcontainer_populate \
	plcontainer_function plcontainer_function_gpdb5 plcontainer_test_python \
	plcontainer_test_python_gpdb5 plcontainer_test_anaconda \
	plcontainer_test_r plcontainer_test_r_gpdb5
REGRESS_GPDB4 = plcontainer_install plcontainer_schema plcontainer_populate \
	plcontainer_function plcontainer_test_python plcontainer_test_anaconda \
	plcontainer_test_r
PSQLDIR = $(bindir)

.PHONY: default
default: all

.PHONY: all
all: tests

.PHONY: tests
tests:
	$(top_builddir)/src/test/regress/pg_regress \
				--psqldir=$(PSQLDIR) $(REGRESS_OPTS) $(REGRESS_GPDB5)

.PHONY: tests4
tests4:
	$(top_builddir)/src/test/regress/pg_regress \
				--psqldir=$(PSQLDIR) $(REGRESS_OPTS) $(REGRESS_GPDB4)

.PHONY: submake
submake:
	$(MAKE) -C $(top_builddir)/src/test/regress pg_regress$(X)
