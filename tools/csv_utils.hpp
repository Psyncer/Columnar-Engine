// #include "parse_error.hpp"

// #include <expected>
#include <sstream>
#include <string>
#include <vector>

namespace columnar {

inline std::vector<std::string> split(const std::string& line, char delimiter) {
    // без экранированных символов
    std::vector<std::string> tokens;
    std::stringstream ss(line);
    std::string token;

    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}

}  // namespace columnar
