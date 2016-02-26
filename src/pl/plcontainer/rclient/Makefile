R_CFLAGS = $(shell pkg-config --cflags libR)
R_LDLAGS = $(shell pkg-config --libs libR)
override CFLAGS += $(DEBUG) -I../ -I../../../include -DCOMM_STANDALONE -Wall -Wextra -Werror

common_src = $(shell find ../common -name "*.c")
common_objs = $(foreach src,$(common_src),$(subst .c,.o,$(src)))

.PHONY: default
default: clean all

.PHONY: clean_common
clean_common:
	rm -f $(common_objs)

%.o: %.c
	$(CC) $(R_CFLAGS) $(CFLAGS) -c -o $@ $^

librsupport.so: rsupport.c 
	$(CC) $(R_CFLAGS) $(CFLAGS) -fpic -c -o rsupport.o $^
	$(CC) -shared -o librsupport.so rsupport.o

client: client.o rcall.o librsupport.so $(common_objs)
	$(CC) -o $@ $^ $(R_LDLAGS)

debug: DEBUG = -D_DEBUG_CLIENT -g

debug: client 

.PHONY: all
all: client

clean: clean_common
	rm -f *.o
	rm -f client
