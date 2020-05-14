#ifndef CECFORWARDER_IRREADER_H
#define CECFORWARDER_IRREADER_H

#include <atomic>
#include <mutex>

#include <p8-platform/os.h>
#include <p8-platform/util/StringUtils.h>
#include <p8-platform/threads/threads.h>

#include "keyname.h"
#include "lircpp.h"

class IRReader : public P8PLATFORM::CThread
{
public:
    class Callback
    {
    public:
        virtual void onReceive(const KeyName& key) = 0;
    };
public:
    IRReader(const std::string& baseDir, const std::string& keyname, bool recordOnly = false);
    virtual ~IRReader(void) {}

    void setVerbose(bool v);

    void addCallback(Callback* cb);

    void cancel();

    void* Process(void) override;

private:
    std::atomic<bool> mVerbose;
    std::atomic<bool> mRunning;

    bool mRecordOnly;
    LircPP mLirc;

    std::vector<Callback*> mCallbacks;
};

#endif // CECFORWARDER_IRREADER_H