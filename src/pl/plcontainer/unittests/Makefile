.PHONY: default
default: run-tests

top_builddir = ../../..

.PHONY: googletest
googletest:
	$(MAKE) -C googletest/googletest/make

PYTHON_CFLAGS = $(shell pkg-config --cflags python-2.7)
PYTHON_LDLAGS = $(shell pkg-config --libs python-2.7)

objs := $(foreach file,$(wildcard *.cpp),$(subst .cpp,.o,$(file)))
depssrc := $(wildcard $(top_builddir)/pl/plcontainer/common/*.c)
depssrc += $(top_builddir)/pl/plcontainer/containers.c
deps := $(foreach src,$(depssrc),$(subst .c,.o,$(src)))

override CFLAGS += -g -I googletest/googletest/include -I $(top_builddir)/include \
			-I $(top_builddir)/pl/plcontainer -DCOMM_STANDALONE -Wno-write-strings

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

%.o: %.cpp
	$(CXX) $(PYTHON_CFLAGS) $(CFLAGS) -c -o $@ $^

test: $(objs) $(deps) googletest
	$(CXX) -o $@ googletest/googletest/make/gtest-all.o $(objs) $(deps) $(PYTHON_LDLAGS) -lpthread

run-tests: test
	./test

clean:
	rm -f *.o
	rm -f test
	rm -f $(deps)
