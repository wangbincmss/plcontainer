# Global build Directories

PLCONTAINER_DIR = ..

# Python build flags
PYTHON_CFLAGS = $(shell pkg-config --cflags python-2.7)
PYTHON_LDFLAGS = $(shell pkg-config --libs python-2.7)

override CFLAGS += $(DEBUG) -I$(PLCONTAINER_DIR)/ -DCOMM_STANDALONE -Wall -Wextra -Werror

common_src = $(shell find $(PLCONTAINER_DIR)/common -name "*.c")
common_objs = $(foreach src,$(common_src),$(subst .c,.o,$(src)))
python_src = $(shell find . -name "*.c")
python_objs = $(foreach src,$(python_src),$(subst .c,.o,$(src)))

.PHONY: default
default: clean all

.PHONY: clean_common
clean_common:
	rm -f $(common_objs)

%.o: %.c
	$(CC) $(PYTHON_CFLAGS) $(CFLAGS) -c -o $@ $^

client: $(python_objs) $(common_objs)
	LIBRARY_PATH=$(LD_LIBRARY_PATH) $(CC) -o $@ $^ $(PYTHON_LDFLAGS) $(LIBS)
	cp client bin

debug: DEBUG = -D_DEBUG_CLIENT -g

debug: client

.PHONY: all
all: client

clean: clean_common
	rm -f *.o
	rm -f client
	rm -f bin/client
