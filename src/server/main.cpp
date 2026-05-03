#include <iostream>
#include <cstdlib>
#include <cstring>
#include "game_engine.hpp"

// Usage: QuantumPong --port PORT
int main(int argc, char *argv[]) {
    unsigned port = 8080;

    for (int i = 1; i < argc - 1; ++i) {
        if (strcmp(argv[i], "--port") == 0) port = (unsigned)atoi(argv[i+1]);
    }

    game_engine eng;
    eng.init(port);
    eng.run();
    eng.finalize();

    return 0;
}
