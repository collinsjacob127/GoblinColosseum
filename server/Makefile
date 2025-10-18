# Author: Jacob Collins

TARGETS = server.x
OBJECT_TARGETS = util.o
CPP_FILES = server.cpp

HEADER_FILES = ${OBJECT_TARGETS:.o=.hpp}
CPP_HEADERS = ${OBJECT_TARGETS:.o=.cpp}

CFLAGS = -Wall -g
CXXFLAGS = -Wall

LDLIBS = 
CXX = gcc
CXX = g++

.PHONY: all
all: $(TARGETS)

# Implicit rules defined by Make, but you can redefine if needed
$(OBJECT_TARGETS): $(HEADER_FILES) $(CPP_HEADERS)
	$(CXX) -c $(CPP_HEADERS)
$(TARGETS): $(CPP_FILES) $(OBJECT_TARGETS)
	$(CXX) $(CFLAGS) $(CPP_FILES) $(HPP_FILES) -lm $(LDLIBS) -o $(TARGETS) $(OBJECT_TARGETS)

# OR

#h1-counter: h1-counter.cc
#	$(CXX) $(CXXFLAGS) h1-counter.cc $(LDLIBS) -o h1-counter

.PHONY: clean
clean:
	rm -f $(TARGETS) $(OBJECT_TARGETS)
