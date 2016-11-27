CXXFLAGS += -Isrc/ -Itest/ -g -std=c++11
LDFLAGS  += -g

SRCS = $(shell find src -name '*.cpp')
TEST_SRCS = $(shell find test -name '*.cpp')

OBJS = $(subst .cpp,.o,$(SRCS))
TEST_OBJS = $(subst .cpp,.o,$(TEST_SRCS))

.PHONY: all makedir test clean dist-clean

all: makedir test

makedir:
	mkdir -p bin

test: bin/test_all
	bin/test_all

bin/test_all: $(OBJS) $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

depend: .depend
.depend: $(SRCS) $(TEST_SRCS)
	rm -f ./.depend
	$(CXX) $(CXXFLAGS) -MM $^ >> ./.depend

clean:
	rm -f $(OBJS) $(TEST_OBJS)

dist-clean: clean
	rm -f *~ .depend

include .depend
