// #include "parse_error.hpp"

// #include <expected>
#include <string>
#include <vector>

namespace columnar {

inline std::vector<std::string> split(const std::string& line, char delimiter) {
    std::vector<std::string> tokens;
    bool is_in_quotes = false;
    std::string current;

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];

        if (is_in_quotes) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    current += c;
                    i++;
                } else {
                    is_in_quotes = false;
                }
            } else {
                current += c;
            }
        } else {
            if (c == '"' && current.empty()) {
                is_in_quotes = true;
            } else if (c == delimiter) {
                tokens.push_back(current);
                current.erase();
            } else {
                current += c;
            }
        }
    }

    if (!current.empty()) {
        tokens.push_back(current);
    }

    // assert tokens.size() == number of columns (?) to catch invalid quotes format early
    return tokens;
}

}  // namespace columnar
