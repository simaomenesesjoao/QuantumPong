# Generated with ChatGPT

# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -Wall -Wextra -I../src -I../src/client -I../src/server `sdl2-config --libs --cflags` -lm -lSDL2_ttf

# Linker flags
LDFLAGS=-lOpenCL

# Source files
SRC_SRC = $(wildcard ../src/*.cpp)
CLIENT_SRC = $(wildcard ../src/client/*.cpp)
SERVER_SRC = $(wildcard ../src/server/*.cpp)
TEST_SRC = test1.cpp

# Excluded files
EXCLUDE_FILES = ../src/client/main.cpp ../src/server/main.cpp ../src/server/kernel.cpp

# Filtered source files
SRC_SRC := $(filter-out $(EXCLUDE_FILES), $(SRC_SRC))
CLIENT_SRC := $(filter-out $(EXCLUDE_FILES), $(CLIENT_SRC))
SERVER_SRC := $(filter-out $(EXCLUDE_FILES), $(SERVER_SRC))

# Object files
SRC_OBJ = $(SRC_SRC:../src/%.cpp=../src/%.o)
CLIENT_OBJ = $(CLIENT_SRC:../src/client/%.cpp=../src/client/%.o)
SERVER_OBJ = $(SERVER_SRC:../src/server/%.cpp=../src/server/%.o)
TEST_OBJ = $(TEST_SRC:%.cpp=%.o)

# Executable target
TARGET = test1

# Default target
all: $(TARGET)

# Build the executable
$(TARGET): $(SRC_OBJ) $(CLIENT_OBJ) $(SERVER_OBJ) $(TEST_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Compile src source files
../src/%.o: ../src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile client source files
../src/client/%.o: ../src/client/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile server source files
../src/server/%.o: ../src/server/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile test source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f $(SRC_OBJ) $(CLIENT_OBJ) $(SERVER

