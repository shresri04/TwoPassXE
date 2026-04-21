CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra 

TARGET = lxe
SRC = assembler.cpp pass2.cpp symtab_output.cpp listing.cpp
HDR = assembler.h

all: $(TARGET)

$(TARGET): $(SRC) $(HDR)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET) *.o *.st *.l *.int
