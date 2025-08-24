#include "include/srd/greeter.hpp"

namespace srd {
std::string Greet(std::string name) {
    return "Hello, " + name + "!";
}
} // namespace srd