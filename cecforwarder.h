#include <unordered_map>
#include <libcec/cec.h>
#include "config.h"
#include "lircpp.h"

struct KeyRepeat {
    CEC::cec_user_control_code keycode;
    uint64_t lastpress;
    uint64_t repeatstart;
};

class CecForwarder
{
public:
    CecForwarder(const std::string& keyname);
    ~CecForwarder();

    void close();
    bool ensureOpen();

    void addKey(int keycode, const std::string& name);
    void setRepeat(int delay, int rate);

private:
    void cecKeyPress(const CEC::cec_keypress* key);
    void cecCommand(const CEC::cec_command* command);
    void cecAlert(const CEC::libcec_alert type, const CEC::libcec_parameter param);

    static void HandleCecKeyPress(void *cbParam, const CEC::cec_keypress* key);
    static void HandleCecCommand(void *cbParam, const CEC::cec_command* command);
    static void HandleCecAlert(void *cbParam, const CEC::libcec_alert type, const CEC::libcec_parameter param);
    static void HandleCecLogMessage(void *cbParam, const CEC::cec_log_message* message);

    std::string mKeyName;
    std::string mBaseDir;
    int mRepeatDelay, mRepeatRate;
    std::unordered_map<int, std::string> mKeys;

    KeyRepeat mKeyRepeat;

    CEC::ICECCallbacks mCecCallbacks;
    CEC::libcec_configuration mCecConfig;
    CEC::ICECAdapter *mAdapter;

    LircPP mLirc;
};
