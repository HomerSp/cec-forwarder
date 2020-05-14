#include <iostream>
#include <fstream>
#include <bitset>
#include <cmath>

#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <linux/lirc.h>

#include "config.h"
#include "lircpp.h"

LircPP::LircPP(const std::string& keyspath)
    : mVerbose(false)
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

void LircPP::setVerbose(bool v)
{
    mVerbose = v;
}

bool LircPP::receive(KeyName& key)
{
    uint32_t k;
    if (receiveRaw(k)) {
        for (auto data: mData) {
            if (data.second == k) {
                key = data.first;
                return true;
            }
        }
    }

    return false;
}

bool LircPP::receiveRaw(uint32_t& value)
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

    close(fd);

    std::vector<unsigned int> flattened;
    for (auto p: data) {
        flattened.push_back(p.second);
    }

    return dataToKey(flattened, value);
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

    if (mVerbose) {
        std::cout << "Sending NEC IR:\n";
        for (uint32_t i = 0; i < sendData.size(); i++) {
            if (i % 2 == 0) {
                std::cout << "pulse ";
            } else {
                std::cout << "space ";
            }

            std::cout << sendData[i] << "\n";
        }
    }

    if (write(fd, sendData.data(), sendData.size() * sizeof(unsigned int)) <= 0) {
        return false;
    }

    close(fd);
    return true;
}

bool LircPP::checkTarget(unsigned int value, unsigned int target)
{
    float diff = 1.0f - (value / static_cast<float>(target));
    return std::abs(floor(diff * 100.0f)) < 25;
}

bool LircPP::dataToKey(const std::vector<unsigned int>& data, uint32_t &value)
{
    value = 0;
    if (data.size() < 5) {
        if (mVerbose) {
            std::cout << "Unhandled IR data with size " << data.size() << "\n";
        }

        return false;
    }

    if (dataToKeyNEC(data, value)) {
        return true;
    }

    if (dataToKeyRC5(data, value)) {
        return true;
    }

    if (mVerbose) {
        std::cout << "Unhandled raw IR:\n";
        for (uint32_t i = 0; i < data.size(); i++) {
            std::cout << data[i] << "\n";
        }

        std::cout << "IR Done\n";
    }

    return false;
}

bool LircPP::dataToKeyNEC(const std::vector<unsigned int>& data, uint32_t& value)
{
    // Header
    if (!checkTarget(data[0], 9000) || !checkTarget(data[1], 4500)) {
        return false;
    }
    
    std::bitset<32> bits;
    for (unsigned int i = 2, shift = 31; i < data.size() - 1; i += 2, shift--) {
        unsigned int pulse = data[i];
        unsigned int space = data[i + 1];
        if (!checkTarget(pulse, 563)) {
            return false;
        }

        if (checkTarget(space, 563)) {
            bits[shift] = 0;
        } else if (checkTarget(space, 1687)) {
            bits[shift] = 1;
        } else {
            return false;
        }
    }

    value = static_cast<uint32_t>(bits.to_ulong());
    return true;
}

bool LircPP::dataToKeyRC5(const std::vector<unsigned int>& data, uint32_t& value)
{
    if (data.size() < 13) {
        return false;
    }

    if (!checkTarget(data[0], 889)) {
        return false;
    }

    std::vector<bool> deflatedData;
    for (unsigned int offset = 1; offset < data.size(); offset++)
    {
        bool pulse = (offset % 2);
        if (checkTarget(data[offset], 889 * 3)) {
            deflatedData.push_back(pulse);
            deflatedData.push_back(pulse);
            deflatedData.push_back(pulse);
        } else if (checkTarget(data[offset], 889 * 2)) {
            deflatedData.push_back(pulse);
            deflatedData.push_back(pulse);
        } else if (checkTarget(data[offset], 889)) {
            deflatedData.push_back(pulse);
        }
    }

    std::bitset<32> bits;
    for (unsigned int i = 0, shift = 31; i < deflatedData.size(); i += 2, shift--) {
        if (!deflatedData[i] && deflatedData[i + 1]) {
            bits[shift--] = 1;
        } else if (deflatedData[i] && !deflatedData[i + 1]) {
            bits[shift--] = 0;
        }
    }

    // We don't care about repeat bit
    bits[29] = 0;

    value = static_cast<uint32_t>(bits.to_ulong());
    return true;
}

bool LircPP::dataToKeyRC6(const std::vector<unsigned int>& data, uint32_t& value)
{
    if (data.size() < 23 || data.size() > 77) {
        return false;
    }

    // Header
    if (!checkTarget(data[0], 444 * 6) || !checkTarget(data[1], 444 * 2)) {
        return false;
    }

    // TODO

    // Header
    /*if (!checkTargetRC6(data[0], 920) || !checkTargetRC6(data[1], 830)) {
        return false;
    }
    
    std::bitset<32> bits;
    for (unsigned int i = 2, shift = 31; i < data.size() - 1; i += 2, shift--) {
        unsigned int pulse = data[i];
        unsigned int space = data[i + 1];
        if (checkTargetRC6(space, 563)) {
            bits[shift] = 0;
        } else if (checkTargetRC6(space, 1687)) {
            bits[shift] = 1;
        } else {
            return false;
        }
    }

    value = static_cast<uint32_t>(bits.to_ulong());
    return true;*/

    return false;
}