# Compiler
#CXX=g++ -std=c++20
CXX=g++

# Compiler flags
CXXFLAGS=`sdl2-config --libs --cflags` -Wall

# Output executable name
TARGET=QuantumPongClient

# Source files
SRCS=../event_queue.cpp front_end.cpp graphics.cpp graphicsEnd.cpp graphicsGame.cpp graphicsLobby.cpp states_connected.cpp states_disconnected.cpp main.cpp



# Object files
OBJS=$(SRCS:.cpp=.o)

LIBS=-lpthread -lm -lSDL2_ttf

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
