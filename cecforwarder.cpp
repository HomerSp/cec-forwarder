#include <iostream>
#include <sys/time.h>
#include "cecforwarder.h"
#include <libcec/cecloader.h>

using namespace CEC;

CecForwarder::CecForwarder(const std::string& baseDir, const std::string& keyname, const std::string& cecname, bool verbose)
    : mVerbose(verbose)
    , mRepeatDelay(850)
    , mRepeatRate(50)
    , mAdapterOpen(false)
    , mAdapter(nullptr)
    , mLirc(baseDir + "/keys/" + keyname)
    , mPower(false)
    , mPowerHome(false)
{
    memset(&mKeyRepeat, 0, sizeof(KeyRepeat));
    mKeyRepeat.keycode = CEC_USER_CONTROL_CODE_UNKNOWN;

    mCecCallbacks.Clear();
    mCecConfig.Clear();

    mCecCallbacks.keyPress        = &CecForwarder::HandleCecKeyPress;
    mCecCallbacks.commandReceived = &CecForwarder::HandleCecCommand;
    mCecCallbacks.alert           = &CecForwarder::HandleCecAlert;
    mCecCallbacks.logMessage      = &CecForwarder::HandleCecLogMessage;

    snprintf(mCecConfig.strDeviceName, LIBCEC_OSD_NAME_SIZE, cecname.c_str());
    mCecConfig.callbackParam = static_cast<void*>(this);
    mCecConfig.clientVersion = CEC::LIBCEC_VERSION_CURRENT;
    mCecConfig.bActivateSource = 0;
    mCecConfig.callbacks = &mCecCallbacks;
    mCecConfig.deviceTypes.Add(CEC::CEC_DEVICE_TYPE_PLAYBACK_DEVICE);

    mAdapter = LibCecInitialise(&mCecConfig);
    if (mAdapter == nullptr) {
        std::cerr << "Failed initialising cec!\n";
        return;
    }

    mAdapter->InitVideoStandalone();
}

CecForwarder::~CecForwarder()
{
    mAdapter = nullptr;
}

void CecForwarder::close()
{
    if (mAdapter != nullptr) {
        mAdapter->Close();
        mAdapterOpen = false;
        UnloadLibCec(mAdapter);
    }
}

bool CecForwarder::ensureOpen()
{
    if (mAdapter == nullptr) {
        return false;
    }

    if (mAdapterOpen) {
        return true;
    }

    std::cerr << "Need to open adapter\n";

    mAdapter->Close();

    bool ret = false;
    CEC::cec_adapter_descriptor devices[10];
    if (mAdapter->DetectAdapters(devices, 10, NULL, true) > 0) {
        ret = mAdapter->Open(devices[0].strComName);
        if (ret) {
            std::cerr << "Opened adapter " << devices[0].strComName << "\n";
        } else {
            std::cerr << "Failed opening adapter " << devices[0].strComName << "\n";
        }
    } else {
        std::cerr << "No adapters found\n";
    }

    mAdapterOpen = ret;
    return ret;
}

void CecForwarder::addKey(int keycode, const std::string& name)
{
    mKeys[keycode] = name;
}

void CecForwarder::setRepeat(int delay, int rate)
{
    mRepeatDelay = (delay > 0) ? delay : mRepeatDelay;
    mRepeatRate = (rate > 0) ? rate : mRepeatRate;
}

void CecForwarder::onReceive(const KeyName& key)
{
    std::cerr << "onReceive " << key.name() << "\n";

    switch (key.value()) {
    case KeyName::KEY_HOME:
        mPowerHome = true;

        mAdapter->PowerOnDevices(CEC::CECDEVICE_TV);
        mAdapter->PowerOnDevices(CEC::CECDEVICE_PLAYBACKDEVICE2);

        mAdapter->SendKeypress(CEC::CECDEVICE_PLAYBACKDEVICE2, CEC::CEC_USER_CONTROL_CODE_POWER_ON_FUNCTION, true);
        mAdapter->SendKeyRelease(CEC::CECDEVICE_PLAYBACKDEVICE2, true);

        mAdapter->SendKeypress(CEC::CECDEVICE_PLAYBACKDEVICE2, CEC::CEC_USER_CONTROL_CODE_ROOT_MENU, true);
        mAdapter->SendKeyRelease(CEC::CECDEVICE_PLAYBACKDEVICE2, true);

        mAdapter->SendKeypress(CEC::CECDEVICE_PLAYBACKDEVICE2, CEC::CEC_USER_CONTROL_CODE_ROOT_MENU, true);
        mAdapter->SendKeyRelease(CEC::CECDEVICE_PLAYBACKDEVICE2, true);

        break;
    }
}

void CecForwarder::cecKeyPress(const CEC::cec_keypress* key)
{
    if (mVerbose) {
        std::cout << "cecKeyPress " << key->keycode << "\n";
    }

    if(key->duration != 0) {
        return;
    }

    timeval tv;
    gettimeofday(&tv, NULL);

    uint64_t timenow = (tv.tv_sec * 1000) + (tv.tv_usec / 1000UL);

    if(mKeyRepeat.keycode == key->keycode) {
        uint64_t diff = timenow - mKeyRepeat.lastpress;

        int repeatDelay = (mKeyRepeat.repeatstart == 0) ? mRepeatDelay : mRepeatRate;
        if(diff < repeatDelay) {
            return;
        }

        std::cerr << "Key repeat " <<  key->keycode << ", diff " << diff << ", delay " << repeatDelay << "\n";
        if (mKeyRepeat.repeatstart == 0) {
            mKeyRepeat.repeatstart = timenow;
        }
    }

    mKeyRepeat.keycode = key->keycode;
    mKeyRepeat.lastpress = timenow;

    if (mKeys.count(key->keycode) > 0) {
        std::string keyName = mKeys.at(key->keycode);
        std::cerr << "Key " << keyName << "\n";
        mLirc.send(keyName);
    }
}

void CecForwarder::cecCommand(const CEC::cec_command* command)
{
    if (mVerbose) {
        std::cout << "cecCommand " << command->opcode << "\n";
    }

    switch(command->opcode) {
    case CEC_OPCODE_USER_CONTROL_PRESSED:
        if(command->parameters.size > 0) {
            cec_keypress key = {(cec_user_control_code) command->parameters.data[0], 0};
            cecKeyPress(&key);
        }

        break;
    case CEC_OPCODE_USER_CONTROL_RELEASE:
        memset(&mKeyRepeat, 0, sizeof(KeyRepeat));
        mKeyRepeat.keycode = CEC_USER_CONTROL_CODE_UNKNOWN;
        break;

    case CEC_OPCODE_ROUTING_CHANGE:
        std::cerr << "Routing change power " << mPower << ", power home " << mPowerHome << "\n";
        if (command->parameters.size == 4) {
            if (command->parameters[0] == 0x00 && command->parameters[1] == 0x00) {
                std::cerr << "Setting power\n";
                mPower = !mPowerHome;
                mPowerHome = false;
            }
        }

        break;
    case CEC_OPCODE_ACTIVE_SOURCE:
        std::cerr << "Active source power " << mPower << ", power home " << mPowerHome << "\n";

        if (mPower && command->parameters.size == 2 && command->parameters[0] != 0x10) {
            std::cerr << "Switching to adapter source\n";
            mAdapter->SetActiveSource();
            mPower = false;
            mPowerHome = false;
        }

        break;
    }
}

void CecForwarder::cecAlert(const CEC::libcec_alert type, const CEC::libcec_parameter param)
{
    if (mVerbose) {
        std::cout << "cecAlert " << type << "\n";
    }

    switch (type) {
    case CEC_ALERT_CONNECTION_LOST:
        std::cerr << "Lost connection, closing adapter\n";
        mAdapterOpen = false;
        break;
    default:
        break;
    }
}

void CecForwarder::cecLogMessage(const CEC::cec_log_message* message)
{
    if (!mVerbose) {
        return;
    }

    static std::unordered_map<CEC::cec_log_level, std::string> sLogLevels = {
        {CEC::CEC_LOG_ERROR, "Error"},
        {CEC::CEC_LOG_WARNING, "Warning"},
        {CEC::CEC_LOG_NOTICE, "Notice"},
        {CEC::CEC_LOG_TRAFFIC, "Traffic"},
        {CEC::CEC_LOG_DEBUG, "Debug"}
    };

    std::string level = (sLogLevels.find(message->level) != sLogLevels.end()) ? sLogLevels.at(message->level) : "Unknown";
    std::cout << level << " [" << message->time << "]: " << message->message << "\n";
}

void CecForwarder::HandleCecKeyPress(void *cbParam, const CEC::cec_keypress* key)
{
    static_cast<CecForwarder*>(cbParam)->cecKeyPress(key);
}

void CecForwarder::HandleCecCommand(void *cbParam, const CEC::cec_command* command)
{
    static_cast<CecForwarder*>(cbParam)->cecCommand(command);
}

void CecForwarder::HandleCecAlert(void *cbParam, const CEC::libcec_alert type, const CEC::libcec_parameter param)
{
    static_cast<CecForwarder*>(cbParam)->cecAlert(type, param);
}

void CecForwarder::HandleCecLogMessage(void *cbParam, const CEC::cec_log_message* message)
{
    static_cast<CecForwarder*>(cbParam)->cecLogMessage(message);
}
