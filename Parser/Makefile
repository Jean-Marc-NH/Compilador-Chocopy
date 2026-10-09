CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic

.PHONY: all test gui clean
all: build/lr1

build/lr1: main.cpp lr1.cpp lr1.hpp
	mkdir -p build
	$(CXX) $(CXXFLAGS) main.cpp lr1.cpp -o $@

test: build/lr1
	python3 ../Tests/testLR1.py --binary ./build/lr1

gui: build/lr1
	python3 gui.py

clean:
	rm -f build/lr1
