#include <iostream>
#include <fstream>
#include <bitset>

#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <linux/lirc.h>

#include "config.h"
#include "lircpp.h"

LircPP::LircPP(const std::string& keyspath)
{
    HueConfig config(keyspath);
    if (!config.parse()) {
        std::cerr << "Failed parsing config\n";
        return;
    }

    HueConfigSection* section = config.getSection("Keys");
    if (section == nullptr) {
        return;
    }

    for (auto it = section->begin(); it != section->end(); it++) {
        KeyName key(it->first);
        if (key != KeyName::KEY_INVALID) {
            mData[key] = strtoul(it->second.c_str(), nullptr, 0);
        }
    }
}

bool LircPP::receive(KeyName& key)
{
    int fd = open("/dev/lirc-rx", O_RDONLY | O_CLOEXEC);
    if (fd == -1) {
        return false;
    }

    struct pollfd fds[1];
    fds[0].fd = fd;
    fds[0].events = POLLIN;

    int mode = LIRC_MODE_MODE2;
    if (ioctl(fd, LIRC_SET_REC_MODE, &mode)) {
        return false;
    }

    std::vector<std::pair<bool, unsigned int> > data;

    std::array<unsigned, 512> buf;

    bool reading = true;
    while (reading) {
        if (poll(fds, 1, 5000) != 1) {
            break;
        }

        ssize_t ret = read(fd, buf.data(), buf.size() * sizeof(unsigned));
        if (ret < 0 && errno == EINTR) {
            continue;
        }

        if (ret == 0 || ret % sizeof(unsigned) != 0) {
            return false;
        }

        for (int i = 0; i < ret / sizeof(unsigned); i++) {
            unsigned val = buf[i] & LIRC_VALUE_MASK;
            unsigned msg = buf[i] & LIRC_MODE2_MASK;

            if (data.size() == 0 && msg == LIRC_MODE2_SPACE) {
                continue;
            }

            if ((msg == LIRC_MODE2_TIMEOUT ||
                (msg == LIRC_MODE2_SPACE && val > 19000))) {
                reading = false;
                break;
            }

            if (msg == LIRC_MODE2_PULSE || msg == LIRC_MODE2_SPACE) {
                bool add = true;
                bool pulse = msg == LIRC_MODE2_PULSE;
                if (data.size() > 0) {
                    if ((data.back().first && pulse) || (!data.back().first && !pulse)) {
                        data.back().second += val;
                        add = false;
                    }
                }

                if (add) {
                    data.push_back({pulse, val});
                }
            }
        }
    }

    bool found = false;

    uint32_t k;
    if (dataToKey(data, k)) {
        for (auto data: mData) {
            if (data.second == k) {
                key = data.first;
                found = true;
                break;
            }
        }
    }

    close(fd);
    return found;
}

bool LircPP::send(const KeyName& key)
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

    std::vector<unsigned int> sendData;
    sendData.push_back(9000);
    sendData.push_back(4500);

    uint32_t value = mData[key];
    for (uint32_t i = 0; i < 32; i++) {
        sendData.push_back(563);
        if ((value >> (32 - i - 1)) & 1U) {
            sendData.push_back(1687);
        } else {
            sendData.push_back(563);
        }
    }

    sendData.push_back(563);

    if (write(fd, sendData.data(), sendData.size() * sizeof(unsigned int)) <= 0) {
        return false;
    }

    close(fd);
    return true;
}

bool LircPP::checkTarget(unsigned int value, unsigned int target)
{
    int diff = value - target;
    return std::abs(diff) < 500;
}

bool LircPP::dataToKey(const std::vector<std::pair<bool, unsigned int> >& data, uint32_t &value)
{
    value = 0;
    if (data.size() < 66) {
        return false;
    }

    // Header
    if (!checkTarget(data[0].second, 9000) || !checkTarget(data[1].second, 4500)) {
        std::cerr << "Invalid header\n";
        return false;
    }

    std::bitset<32> bits;
    for (unsigned int i = 2, shift = 31; i < 66; i += 2, shift--) {
        unsigned int pulse = data[i].second;
        unsigned int space = data[i + 1].second;
        if (!checkTarget(pulse, 563)) {
            std::cerr << "Invalid pulse\n";
            return false;
        }

        if (checkTarget(space, 563)) {
            bits[shift] = 0;
        } else if (checkTarget(space, 1687)) {
            bits[shift] = 1;
        } else {
            std::cerr << "Invalid space\n";
            return false;
        }
    }

    value = static_cast<uint32_t>(bits.to_ulong());
    return true;
}
