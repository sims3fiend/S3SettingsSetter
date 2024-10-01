#include "game_config.h"
#include "utils.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>

GameConfig& GameConfig::Instance() {
    static GameConfig instance;
    return instance;
}

GameConfig::GameConfig() {
    LoadFromFile();
}

void GameConfig::AddOrUpdateConfig(const std::string& key, const std::variant<bool, std::string>& value, uintptr_t address) {
    configs[key] = { key, value, address };
}


std::variant<bool, std::string> GameConfig::GetConfigValue(const std::string& key, const std::variant<bool, std::string>& defaultValue) const {
    auto it = configs.find(key);
    return (it != configs.end()) ? it->second.value : defaultValue;
}

void GameConfig::SetConfigValue(const std::string& key, const std::variant<bool, std::string>& value) {
    auto it = configs.find(key);
    if (it != configs.end()) {
        it->second.value = value;
    }
}

const std::unordered_map<std::string, GameConfigEntry>& GameConfig::GetAllConfigs() const {
    return configs;
}

void GameConfig::SaveToFile() const {
    std::ofstream file(configFilePath);
    if (file.is_open()) {
        for (const auto& [key, entry] : configs) {
            file << key << "=";
            if (std::holds_alternative<bool>(entry.value)) {
                file << (std::get<bool>(entry.value) ? "1" : "0");
            }
            else {
                file << std::get<std::string>(entry.value);
            }
            file << "," << std::hex << entry.address << std::endl;
        }
        file.close();
        Log("Game config saved to file: " + configFilePath);
    }
    else {
        Log("Failed to save game config to file: " + configFilePath);
    }
}

void GameConfig::LoadFromFile() {
    std::ifstream file(configFilePath);
    if (file.is_open()) {
        configs.clear();
        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string key, value, addressStr;
            if (std::getline(iss, key, '=') && std::getline(iss, value, ',') &&
                std::getline(iss, addressStr)) {
                uintptr_t address = std::stoull(addressStr, nullptr, 16);
                if (value == "0" || value == "1") {
                    bool boolValue = (value == "1");
                    configs[key] = { key, boolValue, address };
                }
                else {
                    configs[key] = { key, value, address };
                }
            }
        }
        file.close();
        Log("Game config loaded from file: " + configFilePath);
    }
    else {
        Log("No existing game config file found. A new one will be created when config is saved.");
    }
}

void GameConfig::AnalyzeAndSortConfigs() {
    std::vector<GameConfigEntry> sortedConfigs;
    for (const auto& [key, info] : configs) {
        sortedConfigs.push_back(info);
    }

    std::sort(sortedConfigs.begin(), sortedConfigs.end(),
        [](const GameConfigEntry& a, const GameConfigEntry& b) {
            return a.address < b.address;
        });

    const uintptr_t proximityThreshold = 0x1000;
    std::vector<std::vector<GameConfigEntry>> configGroups;

    for (const auto& config : sortedConfigs) {
        if (configGroups.empty() || config.address - configGroups.back().back().address > proximityThreshold) {
            configGroups.push_back({ config });
        }
        else {
            configGroups.back().push_back(config);
        }
    }

    AnalyzeConfigGroups(configGroups);

    // Log the sorted configs
    std::stringstream ss;
    ss << "Sorted Configs:" << std::endl;
    ss << std::setw(40) << std::left << "Key" << std::setw(20) << "Value" << "Address" << std::endl;
    ss << std::string(70, '-') << std::endl;

    for (const auto& info : sortedConfigs) {
        ss << std::setw(40) << std::left << info.key << std::setw(20);
        if (std::holds_alternative<bool>(info.value)) {
            ss << (std::get<bool>(info.value) ? "true" : "false");
        }
        else {
            ss << std::get<std::string>(info.value);
        }
        ss << "0x" << std::hex << std::setw(8) << std::setfill('0') << info.address << std::endl;
    }

    Log(ss.str());

    SaveToFile();
}

void GameConfig::AnalyzeConfigGroups(const std::vector<std::vector<GameConfigEntry>>& configGroups) {
    std::stringstream ss;
    ss << "Config Group Analysis:" << std::endl;

    std::vector<std::vector<GameConfigEntry>> mergedGroups;
    const uintptr_t mergeThreshold = 0x1000; // Adjust this value to control group merging

    for (const auto& group : configGroups) {
        if (mergedGroups.empty() ||
            group.front().address - mergedGroups.back().back().address > mergeThreshold) {
            mergedGroups.push_back(group);
        }
        else {
            mergedGroups.back().insert(mergedGroups.back().end(), group.begin(), group.end());
        }
    }

    for (size_t i = 0; i < mergedGroups.size(); ++i) {
        const auto& group = mergedGroups[i];
        ss << "Group " << i + 1 << ":" << std::endl;
        ss << "  Start address: 0x" << std::hex << group.front().address << std::endl;
        ss << "  End address: 0x" << std::hex << group.back().address << std::endl;
        ss << "  Size: " << std::dec << group.size() << " configs" << std::endl;
        ss << "  Address range: 0x" << std::hex << (group.back().address - group.front().address) << std::endl;

        std::string commonPrefix = FindCommonPrefix(group);
        if (!commonPrefix.empty()) {
            ss << "  Common prefix: " << commonPrefix << std::endl;
        }

        AnalyzeGroupTypes(ss, group);

        ss << "  Configs:" << std::endl;
        for (const auto& config : group) {
            ss << "    " << std::setw(40) << std::left << config.key << ": ";
            if (std::holds_alternative<bool>(config.value)) {
                ss << (std::get<bool>(config.value) ? "true" : "false");
            }
            else {
                ss << std::get<std::string>(config.value);
            }
            ss << " (0x" << std::hex << config.address << ")" << std::endl;
        }

        ss << std::endl;
    }

    Log(ss.str());
}

std::string GameConfig::FindCommonPrefix(const std::vector<GameConfigEntry>& group) {
    if (group.empty()) return "";
    std::string commonPrefix = group.front().key;
    for (const auto& config : group) {
        while (config.key.substr(0, commonPrefix.length()) != commonPrefix) {
            commonPrefix = commonPrefix.substr(0, commonPrefix.length() - 1);
            if (commonPrefix.empty()) return "";
        }
    }
    return commonPrefix;
}

void GameConfig::AnalyzeGroupTypes(std::stringstream& ss, const std::vector<GameConfigEntry>& group) {
    int boolCount = 0, stringCount = 0, numericCount = 0;
    for (const auto& config : group) {
        if (std::holds_alternative<bool>(config.value)) {
            boolCount++;
        }
        else {
            const std::string& strValue = std::get<std::string>(config.value);
            if (strValue.find_first_not_of("0123456789.-") == std::string::npos) {
                numericCount++;
            }
            else {
                stringCount++;
            }
        }
    }
    ss << "  Type distribution: " << boolCount << " boolean, "
        << numericCount << " numeric, " << stringCount << " string" << std::endl;
}
