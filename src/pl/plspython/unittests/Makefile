.PHONY: default
default: run-tests

top_builddir = ../../..

.PHONY: googletest
googletest:
	$(MAKE) -C googletest/googletest/make

objs := $(foreach file,$(wildcard *.cpp),$(subst .cpp,.o,$(file)))
depssrc := $(wildcard $(top_builddir)/pl/plspython/common/*.c)
depssrc += $(top_builddir)/pl/plspython/containers.c
deps := $(foreach src,$(depssrc),$(subst .c,.o,$(src)))

override CFLAGS += -I googletest/googletest/include -I $(top_builddir)/include \
								-I $(top_builddir)/pl/plspython -DCOMM_STANDALONE

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

%.o: %.cpp
	$(CXX) $(CFLAGS) -c -o $@ $^

test: $(objs) $(deps) googletest
	$(CXX) -o $@ googletest/googletest/make/gtest-all.o $(objs) $(deps) -lpthread

run-tests: test
	./test

clean:
	rm -f *.o
	rm -f test
	rm -f $(deps)
