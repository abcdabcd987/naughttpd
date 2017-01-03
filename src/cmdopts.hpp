#include <map>
#include <string>

class CommandLineOptions {
    std::map<std::string, std::string> arg;
public:
    CommandLineOptions(int argc, char** argv);
    std::string get(const std::string &key, const std::string &def = "") const;
};
