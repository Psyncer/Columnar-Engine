#pragma once

#include <iostream>

#define ASS(cond, msg)                                                                             \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            std::cerr << "Assertion failed: (" #cond ")" << "\n  message: " << msg << "\n  at "    \
                      << __FILE__ << ":" << __LINE__ << "\n  in " << __func__ << std::endl;        \
            std::abort();                                                                          \
        }                                                                                          \
    } while (0)
