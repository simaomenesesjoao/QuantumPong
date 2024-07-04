# Compiler
CXX=g++

# Compiler flags
CXXFLAGS=-Wall 

# Output executable name
TARGET=QuantumPong

# Source files
SRCS=connection_handler.cpp ../event_queue.cpp game_engine.cpp state_machine_player.cpp state_machine_server.cpp simulator.cpp main.cpp


# Object files
OBJS=$(SRCS:.cpp=.o)

LIBS=-lOpenCL

# Default target
all: $(TARGET)

# Linking
$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

# Compiling source files into object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up
clean:
	rm -f $(OBJS) $(TARGET)

# Phony targets
.PHONY: all clean
