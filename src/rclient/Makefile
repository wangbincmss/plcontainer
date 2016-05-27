# Makefile for PL/Container R client

ifeq (,${R_HOME})
#R_HOME is not defined

default: 
	@echo ""; \
	 echo "*** Cannot build PL/Container R client because R_HOME cannot be found." ; \
	 echo "*** Refer to the documentation for details."; \
	 echo ""

else
#R_HOME is defined

# Global build Directories

PLCONTAINER_DIR = ..

# R build flags
#R_CFLAGS = $(shell pkg-config --cflags libR)
#R_LDLAGS = $(shell pkg-config --libs libR)
R_CFLAGS = -I${R_HOME}/include
R_LDLAGS = -Wl,--export-dynamic -fopenmp -Wl,-z,relro -L${R_HOME}/lib -lR

override CFLAGS += $(CUSTOMFLAGS) -I$(PLCONTAINER_DIR)/ -DCOMM_STANDALONE -Wall -Wextra -Werror

common_src = $(shell find $(PLCONTAINER_DIR)/common -name "*.c")
common_objs = $(foreach src,$(common_src),$(subst .c,.o,$(src)))
shared_src = rcall.c rconversions.c rlogging.c

.PHONY: default
default: clean all

.PHONY: clean_common
clean_common:
	rm -f $(common_objs)
	rm -f librcall.so
	rm -f bin/librcall.so

%.o: %.c
	$(CC) $(R_CFLAGS) $(CFLAGS) -c -o $@ $^

librcall.so: $(shared_src)
	$(CC) $(R_CFLAGS) $(CFLAGS) -fpic -c $(shared_src) $^
	$(CC) -shared -o librcall.so rcall.o rconversions.o rlogging.o
	cp librcall.so bin

client: client.o librcall.so $(common_objs)
	$(CC) -o $@ $^ $(R_LDLAGS)
	cp client bin

.PHONY: debug
debug: CUSTOMFLAGS = -D_DEBUG_CLIENT -g -O0
debug: client

.PHONY: all
all: CUSTOMFLAGS = -O3
all: client

clean: clean_common
	rm -f *.o
	rm -f client
	rm -f bin/client

endif # R_HOME check
