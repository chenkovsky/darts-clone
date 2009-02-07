CC = g++
CFLAGS = -Wall -O3

MKDARTS-SRCS = mkdarts-clone.cpp
MKDARTS-OBJS = ${MKDARTS-SRCS:.cpp=.o}
MKDARTS = mkdarts-clone

DARTS-SRCS = darts-clone.cpp
DARTS-OBJS = ${DARTS-SRCS:.cpp=.o}
DARTS = darts-clone

TEST-SRCS = darts-clone-test.cpp
TEST-OBJS = ${TEST-SRCS:.cpp=.o}
TEST = darts-clone-test


.cpp.o:
	${CC} -c ${CFLAGS} $<

ALL: ${MKDARTS} ${DARTS} ${TEST}

${MKDARTS}: ${MKDARTS-OBJS}
	${CC} -o $@ ${CFLAGS} ${MKDARTS-OBJS}

${DARTS}: ${DARTS-OBJS}
	${CC} -o $@ ${CFLAGS} ${DARTS-OBJS}

${TEST}: ${TEST-OBJS}
	${CC} -o $@ ${CFLAGS} ${TEST-OBJS}

check: ALL
	(cd tests; sh test-darts-clone.sh)

clean:
	rm -f *.o

darts-clone.o: darts-clone.h darts-clone.cpp
mkdarts-clone.o: darts-clone.h mkdarts-clone.cpp
darts-clone-test.o: darts-clone.h darts-clone-test.cpp
