#include "keyname.h"

std::unordered_map<std::string, KeyName::Value> KeyName::sKeyMap = {
    {"KEY_0", KeyName::KEY_0},
    {"KEY_1", KeyName::KEY_1},
    {"KEY_2", KeyName::KEY_2},
    {"KEY_3", KeyName::KEY_3},
    {"KEY_4", KeyName::KEY_4},
    {"KEY_5", KeyName::KEY_5},
    {"KEY_6", KeyName::KEY_6},
    {"KEY_7", KeyName::KEY_7},
    {"KEY_8", KeyName::KEY_8},
    {"KEY_9", KeyName::KEY_9},
    {"KEY_BACK", KeyName::KEY_BACK},
    {"KEY_BLUE", KeyName::KEY_BLUE},
    {"KEY_CHANNELDOWN", KeyName::KEY_CHANNELDOWN},
    {"KEY_CHANNELUP", KeyName::KEY_CHANNELUP},
    {"KEY_DOWN", KeyName::KEY_DOWN},
    {"KEY_EPG", KeyName::KEY_EPG},
    {"KEY_FASTFORWARD", KeyName::KEY_FASTFORWARD},
    {"KEY_GREEN", KeyName::KEY_GREEN},
    {"KEY_LAST", KeyName::KEY_LAST},
    {"KEY_LEFT", KeyName::KEY_LEFT},
    {"KEY_MENU", KeyName::KEY_MENU},
    {"KEY_MUTE", KeyName::KEY_MUTE},
    {"KEY_OK", KeyName::KEY_OK},
    {"KEY_OPTION", KeyName::KEY_OPTION},
    {"KEY_PAGEDOWN", KeyName::KEY_PAGEDOWN},
    {"KEY_PAGEUP", KeyName::KEY_PAGEUP},
    {"KEY_PLAYPAUSE", KeyName::KEY_PLAYPAUSE},
    {"KEY_POWER", KeyName::KEY_POWER},
    {"KEY_PVR", KeyName::KEY_PVR},
    {"KEY_RADIO", KeyName::KEY_RADIO},
    {"KEY_RECORD", KeyName::KEY_RECORD},
    {"KEY_RED", KeyName::KEY_RED},
    {"KEY_REWIND", KeyName::KEY_REWIND},
    {"KEY_RIGHT", KeyName::KEY_RIGHT},
    {"KEY_STOP", KeyName::KEY_STOP},
    {"KEY_SUBTITLE", KeyName::KEY_SUBTITLE},
    {"KEY_TEXT", KeyName::KEY_TEXT},
    {"KEY_UP", KeyName::KEY_UP},
    {"KEY_VOD", KeyName::KEY_VOD},
    {"KEY_VOLUMEDOWN", KeyName::KEY_VOLUMEDOWN},
    {"KEY_VOLUMEUP", KeyName::KEY_VOLUMEUP},
    {"KEY_YELLOW", KeyName::KEY_YELLOW},
    {"KEY_HOME", KeyName::KEY_HOME},
};

KeyName::KeyName(const std::string& name)
    : mValue(KeyName::KEY_INVALID)
{
    if (sKeyMap.find(name) != sKeyMap.end()) {
        mValue = sKeyMap[name];
    }
}

KeyName::KeyName(Value v)
    : mValue(v)
{

}

std::string KeyName::name() const
{
    for (auto s: sKeyMap) {
        if (s.second == mValue) {
            return s.first;
        }
    }

    return "";
}
