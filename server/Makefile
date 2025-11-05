# Author: Jacob Collins

TARGETS = server.x
OBJECTS = util.o

OBJ_HEADERS = ${OBJECTS:.o=.hpp}
OBJ_SOURCE = ${OBJECTS:.o=.cpp}

TARGET_SOURCE = ${TARGETS:.x=.cpp}
TARGET_HEADERS = ${TARGETS:.x=.hpp}

CFLAGS = -Wall -g
CXXFLAGS = -Wall

LDLIBS = 
CXX = gcc
CXX = g++

.PHONY: all
all: $(TARGETS)

$(OBJECTS): $(OBJ_SOURCE) $(OBJ_HEADERS) 
	$(CXX) -c $<

$(TARGETS): $(TARGET_SOURCE) $(OBJECTS)
	$(CXX) $(CFLAGS) $(TARGET_SOURCE) -lm $(LDLIBS) -o $(TARGETS) $(OBJECTS)

.PHONY: clean
clean:
	rm -f *.x *.o
