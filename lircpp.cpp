#include <fstream>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/lirc.h>

#include "lircpp.h"

LircPP::LircPP(const std::string& keyspath)
{
    std::ifstream input(keyspath, std::ifstream::in);
    if (!input.good()) {
        return;
    }

    for (std::string name; std::getline(input, name); ) {
        if (name.size() > 0 && name.at(0) == '#') {
            continue;
        }

        std::vector<std::pair<bool, unsigned int> > buffer;
        for (std::string line; std::getline(input, line); ) {
            if (line.empty()) {
                break;
            }

            if (line.size() > 0 && line.at(0) == '#') {
                continue;
            }

            std::string key = line.substr(0, line.find(' '));
            unsigned int data = std::atoi(line.substr(line.find(' ') + 1).c_str());

            if (line.find("space") == 0) {
                if (buffer.size() > 0) {
                    if (!buffer.back().first) {
                        buffer.back().second += data;
                    } else {
                        buffer.push_back({false, data});
                    }
                }
            } else if (line.find("pulse") == 0) {
                if (buffer.size() > 0 && buffer.back().first) {
                    buffer.back().second += data;
                } else {
                    buffer.push_back({true, data});
                }
            } else {
                // Ignore
            }
        }

        mData[name].clear();
        for (auto p: buffer) {
            mData[name].push_back(p.second);
        }
    }
}

bool LircPP::send(const std::string& key)
{
    if (mData.find(key) == mData.end()) {
        return false;
    }

    int fd = open("/dev/lirc-tx", O_WRONLY | O_CLOEXEC);
    if (fd == -1) {
        return false;
    }

    int mode = LIRC_MODE_PULSE;
    if (ioctl(fd, LIRC_SET_SEND_MODE, &mode)) {
        return false;
    }

    if (write(fd, mData[key].data(), mData[key].size() * sizeof(unsigned int)) <= 0) {
        return false;
    }

    close(fd);
    return true;
}
