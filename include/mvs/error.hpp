// include/mvs/error.hpp
#pragma once

#include <string>

namespace mvs {

struct Error {
    std::string message;
    int line;

    std::string toString() const {
        return "Error on line " + std::to_string(line) + ": " + message;
    }
};

} // namespace mvs