#include <iostream>
#include <memory>

// Forward declaration of the Context class
class Context;

// Base state class
class State {
public:
    virtual ~State() {std::cout << "Calling destructor\n";}
    virtual void handle(Context& context) = 0;
};

// Context class that holds the current state
class Context {
private:
    std::unique_ptr<State> currentState;

public:
    Context(std::unique_ptr<State> initialState) : currentState(std::move(initialState)) {}

    void setState(std::unique_ptr<State> newState) {
        currentState = std::move(newState);
    }

    void request() {
        currentState->handle(*this);
    }

};

// Concrete state classes
class StateA : public State {
public:
    void handle(Context& context) override;
};

class StateB : public State {
public:
    void handle(Context& context) override;
};

// Implementation of handle methods
void StateA::handle(Context& context) {
    std::cout << "Handling StateA, switching to StateB\n";
    context.setState(std::make_unique<StateB>());
}

void StateB::handle(Context& context) {
    std::cout << "Handling StateB, switching to StateA\n";
    context.setState(std::make_unique<StateA>());
}


int main() {
    Context context(std::make_unique<StateA>());
    context.request();
    context.request(); 

    return 0;
}
