#pragma once
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <sstream>

struct GameConfigEntry {
    std::string key;
    std::variant<bool, std::string> value;
    uintptr_t address;
//    bool override;
};

class GameConfig {
public:
    // Singleton pattern to access the instance
    static GameConfig& Instance();

    // Methods to add/update and retrieve configuration settings
    void AddOrUpdateConfig(const std::string& key, const std::variant<bool, std::string>& value, uintptr_t address);
    std::variant<bool, std::string> GetConfigValue(const std::string& key, const std::variant<bool, std::string>& defaultValue = false) const;
    void SetConfigValue(const std::string& key, const std::variant<bool, std::string>& value);

    // Retrieve all configurations
    const std::unordered_map<std::string, GameConfigEntry>& GetAllConfigs() const;

    // Load and save configurations to/from file
    void SaveToFile() const;
    void LoadFromFile();

    // Additional analysis and sorting functionality
    void AnalyzeAndSortConfigs();
    void AnalyzeConfigGroups(const std::vector<std::vector<GameConfigEntry>>& configGroups);

private:
    // Private constructor and destructor to enforce singleton
    GameConfig();
    ~GameConfig() = default;

    // Disable copying and assignment to enforce singleton pattern
    GameConfig(const GameConfig&) = delete;
    GameConfig& operator=(const GameConfig&) = delete;

    // Helper methods for analysis
    std::string FindCommonPrefix(const std::vector<GameConfigEntry>& group);
    void AnalyzeGroupTypes(std::stringstream& ss, const std::vector<GameConfigEntry>& group);

    // Internal data storage
    std::unordered_map<std::string, GameConfigEntry> configs;
    const std::string configFilePath = "game_config.txt";
};
