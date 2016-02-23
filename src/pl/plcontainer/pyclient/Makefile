PYTHON_CFLAGS = $(shell pkg-config --cflags python-2.7)
PYTHON_LDLAGS = $(shell pkg-config --libs python-2.7)
override CFLAGS += $(DEBUG) -I../ -I../../../include -DCOMM_STANDALONE -Wall -Wextra -Werror

common_src = $(shell find ../common -name "*.c")
common_objs = $(foreach src,$(common_src),$(subst .c,.o,$(src)))

.PHONY: default
default: clean all

.PHONY: clean_common
clean_common:
	rm -f $(common_objs)

%.o: %.c
	$(CC) $(PYTHON_CFLAGS) $(CFLAGS) -c -o $@ $^

client: client.o pycall.o $(common_objs)
	$(CC) -o $@ $^ $(PYTHON_LDLAGS)

debug: DEBUG = -D_DEBUG_CLIENT -g

debug: client

.PHONY: all
all: client

clean: clean_common
	rm -f *.o
	rm -f client
