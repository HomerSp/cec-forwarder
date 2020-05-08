#ifndef LIRCPP_H
#define LIRCPP_H

#include <unordered_map>
#include <string>
#include <vector>

#include "keyname.h"

class LircPP {
public:
    LircPP(const std::string& keyspath);

    bool receive(KeyName& key);
    bool send(const KeyName& key);

private:
    bool checkTarget(unsigned int value, unsigned int target);
    bool dataToKey(const std::vector<std::pair<bool, unsigned int> >& data, uint32_t &value);

    std::unordered_map<KeyName, uint32_t> mData;
};

#endif //LIRCPP_H