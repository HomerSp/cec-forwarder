#include <iostream>
#include <fstream>
#include <vector>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/lirc.h>

struct LircBuffer {
    bool pulse;
    unsigned int data;
};

bool irSend(const std::string& path) {
    std::ifstream input(path, std::ifstream::in);
    if (!input.good()) {
        std::cerr << "Could not open " << path << "\n";
        return false;
    }

    int fd = open("/dev/lirc-tx", O_WRONLY | O_CLOEXEC);
    if (fd == -1) {
        std::cerr << "Failed to open lirc dev\n";
        return false;
    }

    int mode = LIRC_MODE_PULSE;
    if (ioctl(fd, LIRC_SET_SEND_MODE, &mode)) {
        std::cerr << "Failed to set send mode\n";
        return false;
    }

    std::vector<LircBuffer> buffer;
    for (std::string line; std::getline(input, line); ) {
        std::string key = line.substr(0, line.find(' '));
        unsigned int data = std::atoi(line.substr(line.find(' ') + 1).c_str());

        if (line.find("space") == 0) {
            if (buffer.size() > 0) {
                if (!buffer.back().pulse) {
                    buffer.back().data += data;
                } else {
                    buffer.push_back({false, data});
                }
            }
        } else if (line.find("pulse") == 0) {
            if (buffer.size() > 0 && buffer.back().pulse) {
                buffer.back().data += data;
            } else {
                buffer.push_back({true, data});
            }
        } else {
            // Ignore
        }
    }

    std::vector<unsigned int> data;
    for (auto b: buffer) {
        data.push_back(b.data);
    }

    std::cerr << "Writing " << data.size() << " unsigned ints\n";
    write(fd, data.data(), data.size() * sizeof(unsigned int));

    close(fd);
    return true;
}
