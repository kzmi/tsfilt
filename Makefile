#
# tsfilt
#

CXX= clang++
CXXFLAGS= -O3 --std=c++11 -stdlib=libstdc++ -Wall
LIBS=

TARGET= tsfilt
SOURCES= tsfilt.cpp ts.cpp
HEADERS= ts.h accessor.h

.PHONY: all clean test

all: ${TARGET}

${TARGET} : ${SOURCES} ${HEADERS}
	${CXX} ${CXXFLAGS} ${LIBS} -o $@ $(SOURCES)

clean:
	rm -f $(TARGET)
	cd test; ${MAKE} clean

test:
	cd test; ${MAKE} test
