.PHONY: default
default: run-tests

top_builddir = ../../..

.PHONY: googletest
googletest:
	$(MAKE) -C googletest/googletest/make

PYTHON_CFLAGS = $(shell pkg-config --cflags python-2.7)
PYTHON_LDLAGS = $(shell pkg-config --libs python-2.7)

R_CFLAGS = $(shell pkg-config --cflags libR)
R_LDLAGS = $(shell pkg-config --libs libR)

objs := $(foreach file,$(wildcard *.cpp),$(subst .cpp,.o,$(file)))
depssrc := $(wildcard $(top_builddir)/pl/plcontainer/common/*.c)
depssrc += $(top_builddir)/pl/plcontainer/containers.c
depssrc += $(top_builddir)/pl/plcontainer/rclient/rcall.c

deps := $(foreach src,$(depssrc),$(subst .c,.o,$(src)))

override CFLAGS += -g -I googletest/googletest/include -I $(top_builddir)/include \
			-I $(top_builddir)/pl/plcontainer -DCOMM_STANDALONE -Wno-write-strings

%.o: %.c
	$(CC) $(R_CFLAGS) $(PYTHON_CFLAGS) $(CFLAGS) -c -o $@ $^

%.o: %.cpp
	$(CXX) $(R_CFLAGS) $(PYTHON_CFLAGS) $(CFLAGS) -c -o $@ $^

librcall.so:
	ln -s ../rclient/librcall.so

test: $(objs) $(deps) librcall.so  googletest
	$(CXX) -o $@ googletest/googletest/make/gtest-all.o $(objs) ../rclient/librcall.so  $(deps) $(R_LDLAGS) $(PYTHON_LDLAGS) -lpthread

run-tests: test
	./test

clean:
	rm -f *.o
	rm -f test
	rm -f $(deps)
