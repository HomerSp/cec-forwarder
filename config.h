#ifndef INCLUDES_HUE_CONFIG_H
#define INCLUDES_HUE_CONFIG_H

#include <string>
#include <vector>
#include <map>
#include <cstdlib>

class HueConfigSection;

class HueConfig {
public:
	HueConfig(const std::string &path);
	~HueConfig();

	bool contains(const std::string& section, const std::string& key);

	HueConfigSection* getSection(const std::string& name, std::string key = "", std::string value = "") const;
	const std::vector<HueConfigSection*> getSections(const std::string& name, std::string key = "", std::string value = "") const;

	HueConfigSection* newSection(const std::string& name);

	bool parse(bool* parseFailure = nullptr);
	bool write();

private:
	std::string mPath;
	std::vector<HueConfigSection* > mSections;

};

class HueConfigSection {
public:
	typedef std::map<std::string, std::string> KeyMap;

	HueConfigSection(std::string name = "");

	const KeyMap::const_iterator begin() const {
		return mValues.begin();
	}

	const KeyMap::const_iterator end() const {
		return mValues.end();
	}

	bool hasKey(const std::string &key, std::string value = "") const;

	const std::string &name() const {
		return mName;
	}

	std::string value(const std::string &key, std::string def = "") const;

	bool boolValue(const std::string& key, bool def = false) const {
		if(!hasKey(key)) {
			return def;
		}

		return value(key) == "true";
	}

	int intValue(const std::string &key, int def = 0) const {
		if(!hasKey(key)) {
			return def;
		}

		return strtoul(value(key).c_str(), nullptr, 0);
	}

	void setValue(const std::string& key, const std::string& value);

	bool parse(const std::string& content);

private:
	std::string mName;
	std::map<std::string, std::string> mValues;
}; 

#endif //INCLUDES_HUE_CONFIG_H
