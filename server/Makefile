# Author: Jacob Collins

TARGETS = server.x
HPP_FILES = utils.hpp
CPP_FILES = server.cpp

CFLAGS = -Wall -g
CXXFLAGS = -Wall

LDLIBS = 
CXX = gcc
CXX = g++

.PHONY: all
all: $(TARGETS)

# Implicit rules defined by Make, but you can redefine if needed

$(TARGETS): $(CPP_FILES)
	$(CXX) $(CFLAGS) $(CPP_FILES) -lm $(LDLIBS) -o $(TARGETS)
#
# OR
#
#h1-counter: h1-counter.cc
#	$(CXX) $(CXXFLAGS) h1-counter.cc $(LDLIBS) -o h1-counter

.PHONY: clean
clean:
	rm -f $(TARGETS)
