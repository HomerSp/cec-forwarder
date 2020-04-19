#include <iostream>
#include <sys/time.h>
#include "cecforwarder.h"
#include "irsend.h"
#include <libcec/cecloader.h>

using namespace CEC;

CecForwarder::CecForwarder()
    : mConfig("/etc/cec-forwarder")
    , mAdapter(nullptr)
{
    if (!mConfig.parse()) {
        std::cerr << "Failed parsing config\n";
        return;
    }

    memset(&mKeyRepeat, 0, sizeof(KeyRepeat));
    mKeyRepeat.keycode = CEC_USER_CONTROL_CODE_UNKNOWN;

    mCecCallbacks.Clear();
    mCecConfig.Clear();

    mCecCallbacks.keyPress        = &CecForwarder::HandleCecKeyPress;
    mCecCallbacks.commandReceived = &CecForwarder::HandleCecCommand;
    mCecCallbacks.alert           = &CecForwarder::HandleCecAlert;
    mCecCallbacks.logMessage      = &CecForwarder::HandleCecLogMessage;

    snprintf(mCecConfig.strDeviceName, 13, "CECForwarder");
    mCecConfig.callbackParam = static_cast<void*>(this);
    mCecConfig.clientVersion = CEC::LIBCEC_VERSION_CURRENT;
    mCecConfig.bActivateSource = 0;
    mCecConfig.callbacks = &mCecCallbacks;
    mCecConfig.deviceTypes.Add(CEC::CEC_DEVICE_TYPE_RECORDING_DEVICE);

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
        UnloadLibCec(mAdapter);
    }
}

bool CecForwarder::process()
{
    return ensureOpen();
}

void CecForwarder::cecKeyPress(const CEC::cec_keypress* key)
{
    if (!ensureOpen()) {
        return;
    }

    if(key->duration != 0) {
        return;
    }

    std::string basedir = "";
    if(mConfig.getSection("Main") != NULL) {
        basedir = mConfig.getSection("Main")->value("irbase");
    }

    if (basedir.length() == 0) {
        std::cerr << "Required irbase parameter missing\n";
        return;
    }

    timeval tv;
    gettimeofday(&tv, NULL);

    uint64_t timenow = (tv.tv_sec * 1000) + (tv.tv_usec / 1000UL);

    if(mKeyRepeat.keycode == key->keycode) {
        uint64_t diff = timenow - mKeyRepeat.lastpress;

        int repeatDelay = (mKeyRepeat.repeatstart == 0) ? 850 : 50;
        if (mKeyRepeat.repeatstart == 0) {
            if (mConfig.contains("Main", "repeatdelay")) {
                repeatDelay = mConfig.getSection("Main")->intValue("repeatdelay");
            }
        } else if (mConfig.contains("Main", "repeatrate")) {
            repeatDelay = mConfig.getSection("Main")->intValue("repeatrate");
        }

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

    std::cerr << "Key " << key->keycode << "\n";

    if(mConfig.getSection("Keys") != NULL) {
        std::string cmd = mConfig.getSection("Keys")->value(std::to_string(key->keycode));
        if(cmd.size() > 0) {
            irSend(basedir + "/" + cmd);
        }
    }
}

void CecForwarder::cecCommand(const CEC::cec_command* command)
{
    if (!ensureOpen()) {
        return;
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
    }
}

void CecForwarder::cecAlert(const CEC::libcec_alert type, const CEC::libcec_parameter param)
{
    if (!ensureOpen()) {
        return;
    }

    switch (type) {
    case CEC_ALERT_CONNECTION_LOST:
        mAdapter->Close();
        break;
    default:
        break;
    }
}

bool CecForwarder::ensureOpen()
{
    if (mAdapter == nullptr) {
        return false;
    }

    if (mAdapter->GetAdapterVendorId() != 0) {
        return true;
    }

    std::cerr << "Need to open adapter\n";

    mAdapter->Close();

    CEC::cec_adapter_descriptor devices[10];
    if (mAdapter->DetectAdapters(devices, 10, NULL, true) > 0) {
        return mAdapter->Open(devices[0].strComName);
    } else {
        std::cerr << "DetectAdapters empty\n";
    }

    return false;
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
}
