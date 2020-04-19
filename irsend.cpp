#include <cstdlib>
#include <fstream>

bool irSend(const std::string& path) {
    if (!std::ifstream(path).good()) {
        return false;
    }

    std::string arg = "ir-ctl -d /dev/lirc-tx --send=" + (std::string(path));
    system(arg.c_str());
}
