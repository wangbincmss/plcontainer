# Global build Directories

PLCONTAINER_DIR = ../

# Python build flags
PYV=$(shell python -c "import sys;t='{v[0]}.{v[1]}'.format(v=list(sys.version_info[:2]));sys.stdout.write(t)")
PYTHON_CFLAGS = $(shell pkg-config --cflags python-$(PYV))
PYTHON_LDFLAGS = $(shell pkg-config --libs python-$(PYV))

override CFLAGS += $(DEBUG) -I$(PLCONTAINER_DIR) -DCOMM_STANDALONE -Wall -Wextra -Werror

common_src = $(shell find ../common -name "*.c")
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
	$(CC) -o $@ $^ $(PYTHON_LDFLAGS)

debug: DEBUG = -D_DEBUG_CLIENT -g

debug: client

.PHONY: all
all: client

clean: clean_common
	rm -f *.o
	rm -f client
