#include "app/App.hpp"

#include <cstdio>
#include <exception>

int main() {
    try {
        tf::App app;
        app.run();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "Fatal error: %s\n", error.what());
        return 1;
    }
    return 0;
}
