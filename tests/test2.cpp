#define CATCH_CONFIG_MAIN 
#include "catch.hpp"

// Example function to test
int factorial(int number) {
    return number <= 1 ? 1 : number * factorial(number - 1);
}

// Test case
TEST_CASE("Factorials are computed", "[factorial]") {
    REQUIRE(factorial(1) == 1);
    REQUIRE(factorial(2) == 2);
    REQUIRE(factorial(3) == 6);
    REQUIRE(factorial(10) == 3628800);
}
