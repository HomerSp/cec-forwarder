#include <iostream>
#include "irreader.h"

IRReader::IRReader(const std::string& baseDir, const std::string& keyname, bool recordOnly)
    : mRunning(true)
    , mRecordOnly(recordOnly)
    , mLirc(baseDir + "/keys/" + keyname)
{
}

void IRReader::setVerbose(bool v)
{
    mLirc.setVerbose(v);
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
        if (mRecordOnly) {
            uint32_t value = 0;
            if (mLirc.receiveRaw(value)) {
                std::cout << "Received " << std::hex << value << "\n";
            }

            continue;
        }

        if (mLirc.receive(key)) {
            for (auto* cb: mCallbacks) {
                cb->onReceive(key);
            }
        }
    }
}