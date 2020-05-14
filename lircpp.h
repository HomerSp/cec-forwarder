#ifndef LIRCPP_H
#define LIRCPP_H

#include <unordered_map>
#include <string>
#include <vector>

#include "keyname.h"

class LircPP {
public:
    LircPP(const std::string& keyspath);

    void setVerbose(bool v);

    bool receive(KeyName& key);
    bool receiveRaw(uint32_t& value);
    bool send(const KeyName& key);

private:
    bool checkTarget(unsigned int value, unsigned int target);

    bool dataToKey(const std::vector<unsigned int>& data, uint32_t &value);
    bool dataToKeyNEC(const std::vector<unsigned int>& data, uint32_t& value);
    bool dataToKeyRC5(const std::vector<unsigned int>& data, uint32_t& value);
    bool dataToKeyRC6(const std::vector<unsigned int>& data, uint32_t& value);

    bool mVerbose;
    std::unordered_map<KeyName, uint32_t> mData;
};

#endif //LIRCPP_H