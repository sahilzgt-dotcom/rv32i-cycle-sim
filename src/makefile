CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Isrc
TARGET := build/sim

all: $(TARGET)

$(TARGET): src/main.cpp src/*.h
	@mkdir -p build
	$(CXX) $(CXXFLAGS) src/main.cpp -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf build