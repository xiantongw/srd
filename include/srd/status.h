#pragma once
#include <string>

namespace srd {
    struct Status
    {
        std::string msg;
        bool ok() const {
            return msg.empty();
        }
    };
}