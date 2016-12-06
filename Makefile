CXXFLAGS += -Isrc/ -Itest/ -g -std=c++11
LDFLAGS  += -g

SRCS = \
	src/engine_epoll.cpp \
	src/engine_poll.cpp \
	src/engine_select.cpp \
	src/http.cpp \
	src/http_request.cpp \
	src/network.cpp \
	src/parser.cpp \

MAIN_SRCS = \
	src/main.cpp \

TEST_SRCS = \
	test/test_parser.cpp \

TEST_MAIN_SRCS = \
	test/test_all.cpp

OBJS = $(subst .cpp,.o,$(SRCS))
MAIN_OBJS = $(subst .cpp,.o,$(MAIN_SRCS))
TEST_OBJS = $(subst .cpp,.o,$(TEST_SRCS))
TEST_MAIN_OBJS = $(subst .cpp,.o,$(TEST_MAIN_SRCS))

.PHONY: all makedir main test clean dist-clean

all: makedir main test

makedir:
	mkdir -p bin

main: makedir bin/naughttpd bin/test_all

test: main
	bin/test_all

bin/naughttpd: $(OBJS) src/main.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

bin/test_all: $(OBJS) $(TEST_OBJS) test/test_all.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

depend: .depend
.depend: $(shell find . -name '*.cpp')
	rm -f ./.depend
	$(CXX) $(CXXFLAGS) -MM $^ >> ./.depend

clean:
	rm -f $(OBJS) $(TEST_OBJS)

dist-clean: clean
	rm -f *~ .depend

include .depend
