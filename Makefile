CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra 

TARGET = lxe
SRC = assembler.cpp
HDR = assembler.h

all: $(TARGET)

$(TARGET): $(SRC) $(HDR)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET) *.o *.int *.st