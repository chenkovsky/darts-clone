CC = g++
CFLAGS = -Wall -O3

MKDARTS_SRCS = mkdarts-clone.cpp
MKDARTS_OBJS = $(MKDARTS_SRCS:.cpp=.o)
MKDARTS = mkdarts-clone

DARTS_SRCS = darts-clone.cpp
DARTS_OBJS = $(DARTS_SRCS:.cpp=.o)
DARTS = darts-clone

TEST_SRCS = darts-clone-test.cpp
TEST_OBJS = $(TEST_SRCS:.cpp=.o)
TEST = darts-clone-test

TIME_SRCS = darts-clone-time.cpp
TIME_OBJS = $(TIME_SRCS:.cpp=.o)
TIME = darts-clone-time

.cpp.o:
	$(CC) -c $(CFLAGS) $<

ALL: $(MKDARTS) $(DARTS) $(TEST) $(TIME)

$(MKDARTS): $(MKDARTS_OBJS)
	$(CC) -o $@ $(CFLAGS) $(MKDARTS_OBJS)

$(DARTS): $(DARTS_OBJS)
	$(CC) -o $@ $(CFLAGS) $(DARTS_OBJS)

$(TEST): $(TEST_OBJS)
	$(CC) -o $@ $(CFLAGS) $(TEST_OBJS)

$(TIME): $(TIME_OBJS)
	$(CC) -o $@ $(CFLAGS) $(TIME_OBJS)

check: ALL
	(cd tests; sh test-darts-clone.sh)

clean:
	rm -f *.o

darts-clone.o: darts-clone.h darts-clone.cpp
mkdarts-clone.o: darts-clone.h mkdarts-clone.cpp
darts-clone-test.o: darts-clone.h darts-clone-test.cpp
darts-clone-time.o: darts-clone.h darts-clone-time.cpp
