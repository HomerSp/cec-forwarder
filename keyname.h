#ifndef CECFORWARDER_KEYS_H 
#define CECFORWARDER_KEYS_H

#include <string>
#include <unordered_map>

class KeyName {
public:
    enum Value {
        KEY_INVALID = -1,
        KEY_0,
        KEY_1,
        KEY_2,
        KEY_3,
        KEY_4,
        KEY_5,
        KEY_6,
        KEY_7,
        KEY_8,
        KEY_9,
        KEY_BACK,
        KEY_BLUE,
        KEY_CHANNELDOWN,
        KEY_CHANNELUP,
        KEY_DOWN,
        KEY_EPG,
        KEY_FASTFORWARD,
        KEY_GREEN,
        KEY_LAST,
        KEY_LEFT,
        KEY_MENU,
        KEY_MUTE,
        KEY_OK,
        KEY_OPTION,
        KEY_PAGEDOWN,
        KEY_PAGEUP,
        KEY_PLAYPAUSE,
        KEY_POWER,
        KEY_PVR,
        KEY_RADIO,
        KEY_RECORD,
        KEY_RED,
        KEY_REWIND,
        KEY_RIGHT,
        KEY_STOP,
        KEY_SUBTITLE,
        KEY_TEXT,
        KEY_UP,
        KEY_VOD,
        KEY_VOLUMEDOWN,
        KEY_VOLUMEUP,
        KEY_YELLOW,
        KEY_HOME,
    };

public:
    KeyName(const std::string& name);
    KeyName(KeyName::Value v = KeyName::KEY_INVALID);

    std::string name() const;
    KeyName::Value value() const { return mValue; }

    operator KeyName::Value () const { return mValue; }

    bool operator==(const KeyName &other) const { return mValue == other.mValue; }

private:
    template<typename T>
    operator T () const;

    KeyName::Value mValue;

    static std::unordered_map<std::string, KeyName::Value> sKeyMap;
};

namespace std
{
    template <>
    struct hash<KeyName>
    {
        size_t operator()(const KeyName& k) const
        {
            return k.value();
        }
    };
}

#endif // CECFORWARDER_KEYS_H
