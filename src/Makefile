SOURCES=$(wildcard *.cpp)
OBJECTS=$(SOURCES:.cpp=.o)
DEPS=$(OBJECTS:.o=.d)

BIN=wildcat

CXX=clang++
CXXFLAGS=-stdlib=libc++ -std=c++0x -Wall -Wextra -MD

ifdef DEBUG
CXXFLAGS:=$(CXXFLAGS) -D__DEBUG__
endif

all: $(OBJECTS) $(BIN)

-include $(DEPS)

%.o: %.cpp %.h
	$(CXX) $(CXXFLAGS) $*.cpp -c -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $*.cpp -c -o $@

$(BIN): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@

.PHONY: clean

clean:
	rm $(OBJECTS) $(DEPS) $(BIN)

