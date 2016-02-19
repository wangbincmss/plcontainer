R_CFLAGS = $(shell pkg-config --cflags libR)
R_LDLAGS = $(shell pkg-config --libs libR)
override CFLAGS += -I../ -I../../../include -DCOMM_STANDALONE -Wall -Wextra -Werror

common_src = $(shell find ../common -name "*.c")
common_objs = $(foreach src,$(common_src),$(subst .c,.o,$(src)))

.PHONY: default
default: all

.PHONY: clean_common
clean_common:
	rm -f $(common_objs)

%.o: %.c
	$(CC) $(R_CFLAGS) $(CFLAGS) -c -o $@ $^

client: client.o rcall.o $(common_objs)
	$(CC) -o $@ $^ $(R_LDLAGS)

.PHONY: all
all: client

clean: clean_common
	rm -f *.o
	rm -f client
