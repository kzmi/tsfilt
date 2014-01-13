#
# tsfilt
#

CXX= clang++
CXXFLAGS= -O0 --std=c++11 -stdlib=libstdc++ -Wall
LIBS=
INCLUDES=-I..

TESTS= accessor-test

.PHONY: all clean test

all: ${TESTS}

test: ${TESTS}
	@for f in ${TESTS}; do ./$$f; done
	  
accessor-test : accessor-test.cpp ../accessor.h
	${CXX} ${CXXFLAGS} ${INCLUDES} ${LIBS} -o $@ $<

clean:
	rm -f ${TESTS}
