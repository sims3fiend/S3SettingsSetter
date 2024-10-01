#include "script_settings.h"
#include "utils.h"
#include <fstream>
#include <sstream>
#include <algorithm>

ScriptSettings& ScriptSettings::Instance() {
    static ScriptSettings instance;
    return instance;
}

ScriptSettings::ScriptSettings() {
    LoadFromFile();
}

void ScriptSettings::AddOrUpdateSetting(const std::string& section, const std::string& key, const std::variant<bool, std::string>& value) {
    std::string fullKey = section + "." + key;
    settings[fullKey] = { value, section };
}

std::variant<bool, std::string> ScriptSettings::GetSettingValue(const std::string& section, const std::string& key, const std::variant<bool, std::string>& defaultValue) const {
    std::string fullKey = section + "." + key;
    auto it = settings.find(fullKey);
    return (it != settings.end()) ? it->second.value : defaultValue;
}

void ScriptSettings::SetSettingValue(const std::string& section, const std::string& key, const std::variant<bool, std::string>& value) {
    std::string fullKey = section + "." + key;
    settings[fullKey] = { value, section };
}

const std::unordered_map<std::string, ScriptSettingEntry>& ScriptSettings::GetAllSettings() const {
    return settings;
}

void ScriptSettings::RemoveSetting(const std::string& section, const std::string& key) {
    std::string fullKey = section + "." + key;
    settings.erase(fullKey);
}

void ScriptSettings::SaveToFile() const {
    std::ofstream file(settingsFilePath);
    if (file.is_open()) {
        std::string currentSection;
        for (const auto& [fullKey, entry] : settings) {
            if (entry.section != currentSection) {
                file << "[" << entry.section << "]" << std::endl;
                currentSection = entry.section;
            }
            file << fullKey.substr(fullKey.find('.') + 1) << "=";
            if (std::holds_alternative<bool>(entry.value)) {
                file << (std::get<bool>(entry.value) ? "true" : "false");
            }
            else {
                file << std::get<std::string>(entry.value);
            }
            file << std::endl;
        }
        file.close();
        Log("Script settings saved to file: " + settingsFilePath);
    }
    else {
        Log("Failed to save script settings to file: " + settingsFilePath);
    }
}

void ScriptSettings::LoadFromFile() {
    std::ifstream file(settingsFilePath);
    if (file.is_open()) {
        settings.clear();
        std::string line;
        std::string currentSection;
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            if (line[0] == '[' && line.back() == ']') {
                currentSection = line.substr(1, line.size() - 2);
            }
            else {
                std::istringstream iss(line);
                std::string key, value;
                if (std::getline(iss, key, '=') && std::getline(iss, value)) {
                    std::string fullKey = currentSection + "." + key;
                    if (value == "true" || value == "false") {
                        bool boolValue = (value == "true");
                        settings[fullKey] = { boolValue, currentSection };
                    }
                    else {
                        settings[fullKey] = { value, currentSection };
                    }
                }
            }
        }
        file.close();
        Log("Script settings loaded from file: " + settingsFilePath);
    }
    else {
        Log("No existing script settings file found. A new one will be created when settings are saved.");
    }
}