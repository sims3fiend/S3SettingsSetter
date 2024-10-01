#pragma once
#include <string>
#include <unordered_map>
#include <variant>

struct ScriptSettingEntry {
    std::variant<bool, std::string> value;
    std::string section;
};

class ScriptSettings {
public:
    static ScriptSettings& Instance();

    void AddOrUpdateSetting(const std::string& section, const std::string& key, const std::variant<bool, std::string>& value);
    std::variant<bool, std::string> GetSettingValue(const std::string& section, const std::string& key, const std::variant<bool, std::string>& defaultValue = false) const;
    void SetSettingValue(const std::string& section, const std::string& key, const std::variant<bool, std::string>& value);

    const std::unordered_map<std::string, ScriptSettingEntry>& GetAllSettings() const;

    void SaveToFile() const;
    void LoadFromFile();
    void RemoveSetting(const std::string& section, const std::string& key);

private:
    ScriptSettings();
    ~ScriptSettings() = default;

    ScriptSettings(const ScriptSettings&) = delete;
    ScriptSettings& operator=(const ScriptSettings&) = delete;

    std::unordered_map<std::string, ScriptSettingEntry> settings;
    const std::string settingsFilePath = "script_settings.ini";
};