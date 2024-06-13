# Compiler
CXX=g++

# Compiler flags
CXXFLAGS=-Wall 

# Output executable name
TARGET=QuantumPong

# Source files
SRCS=connection_handler.cpp ../event_queue.cpp game_engine.cpp game_engine_events.cpp simulator.cpp main.cpp


# Object files
OBJS=$(SRCS:.cpp=.o)

LIBS=-lOpenCL

# Header files
DEPS=connection_handler.hpp event_queue.hpp game_engine.hpp game_engine_events.hpp simulator.hpp main.hpp

# Default target
all: $(TARGET)

# Linking
$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

# Compiling source files into object files
%.o: %.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up
clean:
	rm -f $(OBJS) $(TARGET)

# Phony targets
.PHONY: all clean
