#ifndef LIRCPP_H
#define LIRCPP_H

#include <unordered_map>
#include <string>
#include <vector>

class LircPP {
public:
    LircPP(const std::string& keyspath);

    bool send(const std::string& key);

private:
    std::unordered_map<std::string, std::vector<unsigned int> > mData;
};

#endif //LIRCPP_H