#include <cstdio>
#include <iostream>
#include <fstream>
#include <sstream>
#include "config.h"

HueConfig::HueConfig(const std::string &path)
	: mPath(path)
{

}

HueConfig::~HueConfig() {
	for(size_t i = 0; i < mSections.size(); i++) {
		delete mSections[i];
	}

	mSections.clear();
}

HueConfigSection* HueConfig::getSection(const std::string& name, std::string key, std::string value) const {
	for(size_t i = 0; i < mSections.size(); i++) {
		if(mSections[i]->name() != name) {
			continue;
		}

		if(key.size() == 0 || mSections[i]->hasKey(key, value)) {
			return mSections[i];
		}
	}

	return NULL;
}

const std::vector<HueConfigSection*> HueConfig::getSections(const std::string& name, std::string key, std::string value) const {
	std::vector<HueConfigSection *> ret;
	for(std::vector<HueConfigSection*>::const_iterator it = mSections.begin(); it != mSections.end(); ++it) {
		if((*it)->name() != name) {
			continue;
		}

		if(key.size() == 0 || (*it)->hasKey(key, value)) {
			ret.push_back(*it);
		}
	}

	return ret;
}

HueConfigSection* HueConfig::newSection(const std::string& name) {
	HueConfigSection* section = new HueConfigSection(name);
	mSections.push_back(section);
	return section;
}

bool HueConfig::parse(bool& parseFailure) {
	bool ret = false;

	if(mSections.size() > 0) {
		for(size_t i = 0; i < mSections.size(); i++) {
			delete mSections[i];
		}

		mSections.clear();
	}

	std::ifstream file(mPath.c_str());
	if(!file.good()) {
		return false;
	}
	std::string section = "";
	while(file.good()) {
		std::string buf = "";
		std::getline(file, buf);

		bool haveSection = section.length() > 0;
		if(buf.length() > 0 && ((!haveSection && buf[0] == '[') || (haveSection && buf[0] != '[' && buf[0] != '#'))) {
			section += buf + '\n';
		}

		if(!file.good() || (haveSection && buf.length() > 0 && buf[0] == '[')) {
			if(section.size() > 0) {
				HueConfigSection* configSection = new HueConfigSection();
				if(!configSection->parse(section)) {
					ret = false;
					break;
				}

				mSections.push_back(configSection);
				if(file.good()) {
					section = buf + '\n';
				} else {
					section = "";
				}

				ret = true;
			}
		}
	}

	file.close();

	if(!ret && mSections.size() > 0) {
		for(size_t i = 0; i < mSections.size(); i++) {
			delete mSections[i];
		}

		mSections.clear();
	}

	if(!ret) {
		parseFailure = true;
	}

	return ret;
}

bool HueConfig::write() {
	std::ofstream file(mPath.c_str(), std::ios::trunc);
	if(!file.good()) {
		return false;
	}

	for(std::vector<HueConfigSection*>::const_iterator itSection = mSections.begin(); itSection != mSections.end(); ++itSection) {
		file << "[" << (*itSection)->name() << "]\n";
		for(HueConfigSection::KeyMap::const_iterator it = (*itSection)->begin(); it != (*itSection)->end(); ++it) {
			file << (*it).first << "=" << (*it).second << "\n";
		}

		file << "\n";
	}

	file.close();

	return true;
}

HueConfigSection::HueConfigSection(std::string name)
	: mName(name)
{

}

bool HueConfigSection::hasKey(const std::string &key, std::string value) const {
	std::map<std::string, std::string>::const_iterator it = mValues.find(key);
	if(it != mValues.end()) {
		if(value.length() > 0 && it->second != value) {
			return false;
		}

		return true;
	}

	return false;
}

std::string HueConfigSection::value(const std::string &key, std::string def) const {
	std::map<std::string, std::string>::const_iterator it = mValues.find(key);
	if(it != mValues.end()) {
		return it->second;
	}

	return def;
}

void HueConfigSection::setValue(const std::string& key, const std::string& value) {
	std::map<std::string, std::string>::iterator it = mValues.find(key);
	if(it != mValues.end()) {
		it->second = value;
		return;
	}

	mValues.insert(std::make_pair(key, value));
}

bool HueConfigSection::parse(const std::string& content) {
	std::istringstream iss(content);

	std::string line;
	if(std::getline(iss, line)) {
		size_t pos = line.find(']');
		if(pos == std::string::npos) {
			return false;
		}

		mName = line.substr(1, pos - 1);
		if(mName.size() == 0) {
			return false;
		}
	}

	while(std::getline(iss, line)) {
		if(line.length() > 0 && line[0] == '#') {
			continue;
		}

		size_t pos = line.find('=');
		if(line.size() > 0 && pos == std::string::npos) {
			return false;
		} else if(pos == std::string::npos) {
			continue;
		}

		if(pos == 0) {
			return false;
		}

		mValues.insert(std::make_pair(line.substr(0, pos), line.substr(pos + 1)));
	}

	return true;
}
