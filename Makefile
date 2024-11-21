CXX := g++
CXXFLAGS := -Wall -Wextra -pedantic -lncurses

TARGETS := server client

all: $(TARGETS)

%: %.cpp
	$(CXX) -o $@ $< $(CXXFLAGS)

clean:
	rm -f $(TARGETS)

.PHONY: all clean
