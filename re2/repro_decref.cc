
#include <iostream>
#include <string>
#include "re2/re2.h"
#include "re2/regexp.h"

int main() {
    std::string pattern = "a";
    int depth = 2000;
    for (int i = 0; i < depth; i++) {
        pattern = "(" + pattern + ")a";
    }

    std::cout << "Parsing pattern of depth " << depth << "..." << std::endl;
    re2::RE2::Options options;
    options.set_log_errors(false);
    re2::RE2 re(pattern, options);
    
    if (re.ok()) {
        std::cout << "Success!" << std::endl;
    } else {
        std::cout << "Failed to parse: " << re.error() << std::endl;
    }

    return 0;
}
