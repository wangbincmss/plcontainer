# Global build Directories

PLCONTAINER_DIR = ..

# R build flags
R_CFLAGS = $(shell pkg-config --cflags libR)
R_LDLAGS = $(shell pkg-config --libs libR)

override CFLAGS += $(DEBUG) -I$(PLCONTAINER_DIR)/ -DCOMM_STANDALONE -Wall -Wextra -Werror

common_src = $(shell find $(PLCONTAINER_DIR)/common -name "*.c")
common_objs = $(foreach src,$(common_src),$(subst .c,.o,$(src)))
shared_src = rcall.c rconversions.c

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
	$(CC) -shared -o librcall.so rcall.o rconversions.o
	cp librcall.so bin

client: client.o librcall.so $(common_objs)
	$(CC) -o $@ $^ $(R_LDLAGS)
	mv client bin

debug: DEBUG = -D_DEBUG_CLIENT -g

debug: client 

.PHONY: all
all: client

clean: clean_common
	rm -f *.o
	rm -f client
	rm -f bin/client
