#include <iostream>
#include "irreader.h"

IRReader::IRReader(const std::string& baseDir, const std::string& keyname)
    : mRunning(true)
    , mLirc(baseDir + "/keys/" + keyname)
{

}

void IRReader::addCallback(Callback* cb) {
    mCallbacks.push_back(cb);
}

void IRReader::cancel() {
    mRunning = false;
}

void* IRReader::Process()
{
    KeyName key;
    while (mRunning) {
        if (mLirc.receive(key)) {
            for (auto* cb: mCallbacks) {
                cb->onReceive(key);
            }
        }
    }
}