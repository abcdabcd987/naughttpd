#include "cmdopts.hpp"
#include <cstring>
#include <cstdlib>

void die() {
    fprintf(stderr, "invalid arguments\n");
    exit(EXIT_FAILURE);
}

CommandLineOptions::CommandLineOptions(int argc, char** argv) {
    if (argc % 2 == 0) die();
    for (int i = 1; i < argc; i += 2) {
        if (strncmp(argv[i], "--", 2) != 0) die();
        arg[&argv[i][2]] = argv[i+1];
    }
}

std::string CommandLineOptions::get(const std::string &key, const std::string &def) const {
    auto iter = arg.find(key);
    if (iter == arg.end()) return def;
    return iter->second;
}
