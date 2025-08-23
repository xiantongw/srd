#include "include/srd/greeter.h"

namespace srd {
    std::string Greet(std::string name) {
        return "Hello, " + name + "!";
    }
}