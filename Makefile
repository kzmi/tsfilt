#
# tsfilt
#

CXX= clang++
CXXFLAGS= -O3 --std=c++11 -stdlib=libstdc++ -Wall
LIBS=

TARGET= tsfilt

SOURCES= tsfilt.cpp

all : ${TARGET}

${TARGET} : ${SOURCES}
	${CXX} ${CXXFLAGS} ${LIBS} -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET)
