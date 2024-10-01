//PLEASE do not read any of this code omgggggggggggggggggggg it's so bad
//Half the stuff here is testing that I didn't want to remove in case I needed it later, I should really make two versions but cba

#define NOMINMAX //hate is not a strong enough word
#include "hooks.h"
#include "renderer.h"
#include "gui.h"
#include "utils.h"
#include "game_config.h"
#include "script_settings.h"
#include <MinHook.h>
#include <sstream>
#include <iomanip>
#include <Windows.h>
#include <d3d9.h>
#include <cstring>
#include <vector>
#include <map>
#include <algorithm>
#include <bitset>
#include <locale>
#include <codecvt>
#include <stdexcept>


#include <mutex>

typedef void* (__fastcall* FUN_006ef780_t)(void* param_1);
FUN_006ef780_t original_FUN_006ef780 = nullptr;

typedef HRESULT(__stdcall* EndScene_t)(LPDIRECT3DDEVICE9 pDevice);
EndScene_t original_EndScene = nullptr;
EndScene_t trampoline_EndScene = nullptr; //jump jump trampoline fly to where you want to be

typedef unsigned char(__cdecl* GetConfigBoolWithKeyConstruction_t)(int* param_1, unsigned char param_2);
GetConfigBoolWithKeyConstruction_t original_GetConfigBoolWithKeyConstruction = nullptr;

RetrieveConfigValue_t original_RetrieveConfigValue = nullptr;

std::map<std::string, std::pair<std::string, uintptr_t>> uniqueConfigs;

// graveyard
GetConfigValueAsString_t original_GetConfigValueAsString = nullptr;
FUN_007d02e0_t original_FUN_007d02e0 = nullptr; //BAD BAD BAD 
ParseConfigIni_t original_ParseConfigIni = nullptr;
FUN_004eedf0_t original_FUN_004eedf0 = nullptr;
FUN_007cff40_t original_FUN_007cff40 = nullptr;
FUN_00c12830_t original_FUN_00c12830 = nullptr;
FUN_00c128c0_t original_FUN_00c128c0 = nullptr;
FUN_006c3480_t original_FUN_006c3480 = nullptr;
//rest in peace

//please kill me
std::mutex configMutex;

bool GetD3D9Device(void** pTable, size_t size) {
    if (!pTable)
        return false;

    IDirect3D9* pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (!pD3D)
        return false;

    D3DPRESENT_PARAMETERS d3dpp = {};
    d3dpp.Windowed = FALSE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.hDeviceWindow = GetForegroundWindow();

    IDirect3DDevice9* pDummyDevice = nullptr;
    HRESULT hr = pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDummyDevice);

    if (FAILED(hr) || !pDummyDevice) {
        d3dpp.Windowed = !d3dpp.Windowed;
        hr = pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow,
            D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDummyDevice);
        if (FAILED(hr) || !pDummyDevice) {
            pD3D->Release();
            return false;
        }
    }

    memcpy(pTable, *(void***)(pDummyDevice), size);
    pDummyDevice->Release();
    pD3D->Release();
    return true;
}

bool InitializeHooks() {
    MH_STATUS status = MH_Initialize();
    if (status != MH_OK) {
        Log("MinHook initialization failed: " + std::to_string(static_cast<int>(status)));
        return false;
    }
    Log("MinHook initialized successfully");

    // Hook EndScene
    void** pVTable = new void* [119];
    if (GetD3D9Device(pVTable, sizeof(void*) * 119)) {
        original_EndScene = (EndScene_t)pVTable[42];
        status = MH_CreateHook((LPVOID)original_EndScene, &HookedEndScene, reinterpret_cast<LPVOID*>(&trampoline_EndScene));
        if (status != MH_OK) {
            Log("Failed to create hook for EndScene: " + std::to_string(static_cast<int>(status)));
            delete[] pVTable;
            return false;
        }
        Log("Hook created successfully for EndScene");
    }
    else {
        Log("Failed to get DirectX9 Device");
        delete[] pVTable;
        return false;
    }
    delete[] pVTable;

    // Hook GetConfigBoolWithKeyConstruction
    status = MH_CreateHook(reinterpret_cast<LPVOID>(0x0058b100), &HookedGetConfigBoolWithKeyConstruction, reinterpret_cast<LPVOID*>(&original_GetConfigBoolWithKeyConstruction));
    if (status != MH_OK) {
        Log("Failed to create hook for GetConfigBoolWithKeyConstruction: " + std::to_string(static_cast<int>(status)));
        return false;
    }
    Log("Hook created successfully for GetConfigBoolWithKeyConstruction");

    status = MH_CreateHook(reinterpret_cast<LPVOID>(0x0058c380), &HookedRetrieveConfigValue, reinterpret_cast<LPVOID*>(&original_RetrieveConfigValue));
    if (status != MH_OK) {
        Log("Failed to create hook for RetrieveConfigValue: " + std::to_string(static_cast<int>(status)));
        return false;
    }
    Log("Hook created successfully for RetrieveConfigValue");

    status = MH_EnableHook(MH_ALL_HOOKS);
    if (status != MH_OK) {
        Log("Failed to enable hooks: " + std::to_string(static_cast<int>(status)));
        return false;
    }
    Log("All hooks enabled successfully");

    return true;
}

void CleanupHooks() {
    Log("Cleaning up hooks");
    MH_STATUS status = MH_DisableHook(MH_ALL_HOOKS);
    if (status != MH_OK) {
        Log("Failed to disable hooks: " + std::to_string(static_cast<int>(status)));
    }

    status = MH_Uninitialize();
    if (status != MH_OK) {
        Log("Failed to uninitialize MinHook: " + std::to_string(static_cast<int>(status)));
    }

    // idk
    original_EndScene = nullptr;
    original_GetConfigBoolWithKeyConstruction = nullptr;
    original_RetrieveConfigValue = nullptr;

    Log("Hooks cleanup completed");
}

unsigned char __cdecl HookedGetConfigBoolWithKeyConstruction(int* param_1, unsigned char param_2) {
    char* key = reinterpret_cast<char*>(param_1);
    unsigned char result = original_GetConfigBoolWithKeyConstruction(param_1, param_2);

    GameConfig& config = GameConfig::Instance();
    ScriptSettings& scriptSettings = ScriptSettings::Instance();

    uintptr_t address = reinterpret_cast<uintptr_t>(&result);

    // Check if there's an override in ScriptSettings
    auto& settings = scriptSettings.GetAllSettings();
    auto settingIt = settings.find(key);

    if (settingIt != settings.end()) {
        // Override exists in ScriptSettings
        if (std::holds_alternative<bool>(settingIt->second.value)) {
            bool savedValue = std::get<bool>(settingIt->second.value);
            Log(std::string(key) + " detected near address 0x" + std::to_string(address) +
                ", overriding with saved value: " + (savedValue ? "true" : "false"));
            config.AddOrUpdateConfig(key, savedValue, address);
            return savedValue ? 1 : 0;
        }
        else {
            Log(std::string(key) + " found in ScriptSettings but is not a boolean value. Ignoring override.");
        }
    }
    else {
        // No override, use the original result
        bool resultBool = result != 0;
        config.AddOrUpdateConfig(key, resultBool, address);
        Log(std::string(key) + " not overridden, adding/updating with value: " +
            (resultBool ? "true" : "false") + " near address 0x" + std::to_string(address));
    }

    return result;
}

std::string WideToNarrow(const std::wstring& wstr)
{
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

unsigned int __fastcall HookedRetrieveConfigValue(int param_1_00, int dummy, short* param_2, int* param_3, void** param_4)
{
    std::wstring keyWStr(reinterpret_cast<wchar_t*>(param_3));
    std::string keyStr = WideToNarrow(keyWStr);

    unsigned int result = original_RetrieveConfigValue(param_1_00, param_2, param_3, param_4);

    if (result && param_4 && *param_4)
    {
        std::wstring wideValue(reinterpret_cast<wchar_t*>(*param_4));
        std::string value = WideToNarrow(wideValue);
        uintptr_t address = reinterpret_cast<uintptr_t>(*param_4);

        GameConfig& config = GameConfig::Instance();
        ScriptSettings& scriptSettings = ScriptSettings::Instance();

        // Check if there's an override in ScriptSettings
        auto& settings = scriptSettings.GetAllSettings();
        auto settingIt = settings.find(keyStr);

        if (settingIt != settings.end())
        {
            if (std::holds_alternative<std::string>(settingIt->second.value)) {
                std::string savedValue = std::get<std::string>(settingIt->second.value);
                Log(keyStr + " detected near address 0x" + std::to_string(address) +
                    ", overriding with saved value: " + savedValue);
                config.AddOrUpdateConfig(keyStr, savedValue, address);

                // Copy the overridden value back to *param_4
                std::wstring savedValueWStr(savedValue.begin(), savedValue.end());
                wcscpy_s(reinterpret_cast<wchar_t*>(*param_4),
                    wcslen(reinterpret_cast<wchar_t*>(*param_4)) + 1,
                    savedValueWStr.c_str());
            }
            else {
                Log(keyStr + " found in ScriptSettings but is not a string value. Ignoring override.");
            }
        }
        else
        {
            // No override, use the original value
            config.AddOrUpdateConfig(keyStr, value, address);
            Log(keyStr + " not overridden, adding/updating with value: " + value +
                " near address 0x" + std::to_string(address));
        }

        // Update uniqueConfigs for tracking (if necessary)
        {
            std::lock_guard<std::mutex> lock(configMutex);
            uniqueConfigs[keyStr] = std::make_pair(value, address);
        }
    }

    return result;
}



DWORD __fastcall HookedParseConfigIni(int param_1_00, int dummy, wchar_t* param_2, DWORD param_3) {
    Log("Entering HookedParseConfigIni");

    // Convert wchar_t* to std::string for logging
    std::wstring wstr(param_2);
    std::string str(wstr.begin(), wstr.end());

    Log("ParseConfigIni called with filename: " + str);

    DWORD result = original_ParseConfigIni(param_1_00, param_2, param_3);

    std::stringstream ss;
    ss << "ParseConfigIni result: 0x" << std::hex << result;
    Log(ss.str());

    Log("Exiting HookedParseConfigIni");
    return result;
}

DWORD __fastcall HookedFUN_004eedf0(int param_1_00, int dummy, int param_2) {
    Log("Entering HookedFUN_004eedf0");

    std::stringstream ss;
    ss << "FUN_004eedf0 called with param_1_00: 0x" << std::hex << param_1_00
        << ", param_2: 0x" << param_2;
    Log(ss.str());

    // Log the value at param_1_00 + 0x48c
    DWORD* config_type_ptr = reinterpret_cast<DWORD*>(param_1_00 + 0x48c);
    ss.str("");
    ss << "Value at param_1_00 + 0x48c: 0x" << std::hex << *config_type_ptr;
    Log(ss.str());

    // Examine the structure around 0x434
    ss.str("");
    ss << "Structure around 0x434:" << std::endl;
    for (int i = 0x420; i <= 0x448; i += 4) {
        DWORD* ptr = reinterpret_cast<DWORD*>(param_1_00 + i);
        if (IsBadReadPtr(ptr, sizeof(DWORD))) {
            ss << "  0x" << std::hex << i << ": Unable to read memory" << std::endl;
        }
        else {
            ss << "  0x" << std::hex << i << ": 0x" << *ptr << std::endl;
        }
    }
    Log(ss.str());

    // Check the values at specific offsets
    BYTE* byte_43e = reinterpret_cast<BYTE*>(param_1_00 + 0x43e);
    DWORD* dword_434 = reinterpret_cast<DWORD*>(param_1_00 + 0x434);

    ss.str("");
    ss << "Before call - Value at offset 0x43e: " << (IsBadReadPtr(byte_43e, sizeof(BYTE)) ? "Unable to read" : std::to_string(*byte_43e)) << std::endl;
    ss << "Before call - Value at offset 0x434: " << (IsBadReadPtr(dword_434, sizeof(DWORD)) ? "Unable to read" : ("0x" + std::to_string(*dword_434)));
    Log(ss.str());

    DWORD result = original_FUN_004eedf0(param_1_00, param_2);

    ss.str("");
    ss << "FUN_004eedf0 returned: 0x" << std::hex << result;
    Log(ss.str());

    // Check if the values at 0x43e and 0x434 have changed
    if (!IsBadReadPtr(byte_43e, sizeof(BYTE)) && !IsBadReadPtr(dword_434, sizeof(DWORD))) {
        ss.str("");
        ss << "After call - Value at offset 0x43e: " << std::to_string(*byte_43e) << std::endl;
        ss << "After call - Value at offset 0x434: 0x" << std::hex << *dword_434;
        Log(ss.str());

        // Examine the memory pointed to by the value at 0x434
        DWORD* config_data = reinterpret_cast<DWORD*>(*dword_434);
        if (!IsBadReadPtr(config_data, sizeof(DWORD) * 16)) {
            ss.str("");
            ss << "Configuration data structure:" << std::endl;
            for (int i = 0; i < 16; i++) {
                ss << "  0x" << std::hex << i * 4 << ": 0x" << config_data[i] << std::endl;
            }
            Log(ss.str());

            // Examine the actual configuration entries
            DWORD* entries_ptr = reinterpret_cast<DWORD*>(config_data[10]); // 0x28 offset
            DWORD num_entries = config_data[4]; // 0x10 offset
            if (!IsBadReadPtr(entries_ptr, sizeof(DWORD) * num_entries * 4)) { // Assuming each entry is at least 16 bytes
                ss.str("");
                ss << "Configuration entries:" << std::endl;
                for (DWORD i = 0; i < std::min(num_entries, DWORD(10)); i++) { // Limit to first 10 entries
                    ss << "  Entry " << i << ":" << std::endl;
                    for (int j = 0; j < 4; j++) {
                        ss << "    0x" << std::hex << j * 4 << ": 0x" << entries_ptr[i * 4 + j] << std::endl;
                    }
                }
                Log(ss.str());
            }
            else {
                Log("Unable to read configuration entries");
            }
        }
        else {
            Log("Unable to read configuration data structure");
        }

        Log("Exiting HookedFUN_004eedf0");
        return result;
    }
}

unsigned int __fastcall HookedFUN_007cff40(int param_1_00, int dummy, int* param_2) {
    Log("Entering HookedFUN_007cff40");

    std::stringstream ss;
    ss << "FUN_007cff40 called with param_1_00: 0x" << std::hex << param_1_00
        << ", param_2: 0x" << reinterpret_cast<uintptr_t>(param_2);
    Log(ss.str());

    unsigned int result = original_FUN_007cff40(param_1_00, param_2);

    ss.str("");
    ss << "FUN_007cff40 returned: 0x" << std::hex << result;
    Log(ss.str());

    // Log the values stored at param_1_00 + 0x48c and param_1_00 + 0x490
    DWORD* value_48c = reinterpret_cast<DWORD*>(param_1_00 + 0x48c);
    DWORD* value_490 = reinterpret_cast<DWORD*>(param_1_00 + 0x490);

    if (!IsBadReadPtr(value_48c, sizeof(DWORD)) && !IsBadReadPtr(value_490, sizeof(DWORD))) {
        ss.str("");
        ss << "After call - Value at param_1_00 + 0x48c: 0x" << std::hex << *value_48c << std::endl;
        ss << "After call - Value at param_1_00 + 0x490: 0x" << std::hex << *value_490;
        Log(ss.str());
    }

    Log("Exiting HookedFUN_007cff40");
    return result;
}

void __fastcall HookedFUN_006c3480(void* param_1_00, int dummy, int param_2) {
    Log("Entering HookedFUN_006c3480");

    std::stringstream ss;
    ss << "FUN_006c3480 called with param_1_00: 0x" << std::hex << reinterpret_cast<uintptr_t>(param_1_00)
        << ", param_2: 0x" << param_2;
    Log(ss.str());

    // Log the first 32 values of param_1_00 before the function call
    ss.str("");
    ss << "param_1_00 values before call:" << std::endl;
    float* float_array = static_cast<float*>(param_1_00);
    for (int i = 0; i < 32; i++) {
        ss << "  [" << std::dec << i << "]: 0x" << std::hex << *reinterpret_cast<unsigned int*>(&float_array[i])
            << " (float: " << float_array[i] << ")" << std::endl;
    }
    Log(ss.str());

    original_FUN_006c3480(param_1_00, param_2);

    // Log the values again after the function call
    ss.str("");
    ss << "param_1_00 values after call:" << std::endl;
    for (int i = 0; i < 32; i++) {
        ss << "  [" << std::dec << i << "]: 0x" << std::hex << *reinterpret_cast<unsigned int*>(&float_array[i])
            << " (float: " << float_array[i] << ")" << std::endl;
    }
    Log(ss.str());

    Log("Exiting HookedFUN_006c3480");
}


std::string WideToUTF8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

std::string IntToHexString(int value) {
	std::stringstream ss;
	ss << std::hex << std::setw(8) << std::setfill('0') << value;
	return ss.str();
}

std::string HexDump(const void* data, size_t size) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    const unsigned char* p = static_cast<const unsigned char*>(data);
    for (size_t i = 0; i < size; ++i) {
        ss << std::setw(2) << static_cast<int>(p[i]) << " ";
        if ((i + 1) % 16 == 0) ss << "\n";
    }
    return ss.str();
}

class SEHException : public std::exception {
public:
    SEHException(DWORD code) : m_code(code) {}
    const char* what() const noexcept override {
        static char buf[128];
        sprintf_s(buf, "SEH Exception: 0x%08X", m_code);
        return buf;
    }
private:
    DWORD m_code;
};

// SEH-to-exception wrapper
template<typename Func>
void SEHWrapper(Func func) {
    __try {
        func();
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        throw SEHException(GetExceptionCode());
    }
}

void __cdecl HookedFUN_00c12830(wchar_t** param_1, const wchar_t* param_2, const wchar_t* param_3) {
    try {
        Log("Entering HookedFUN_00c12830");

        std::wstring configSection(param_2);
        std::wstring configKey(param_3);

        std::string narrowConfigSection = WideToUTF8(configSection);
        std::string narrowConfigKey = WideToUTF8(configKey);

        std::stringstream ss;
        ss << "FUN_00c12830 called with section: " << narrowConfigSection
            << ", key: " << narrowConfigKey;
        Log(ss.str());

        Log("About to call original function");
        if (original_FUN_00c12830) {
            SEHWrapper([&]() {
                original_FUN_00c12830(param_1, param_2, param_3);
                });
            Log("Original function call completed");

            if (param_1 && *param_1) {
                ss.str("");
                ss << "Structure for " << narrowConfigSection << "." << narrowConfigKey << ":" << std::endl;

                // Assuming the first 4 bytes are a pointer to the actual data
                uint32_t* dataPtr = reinterpret_cast<uint32_t*>(*param_1);
                if (dataPtr && !IsBadReadPtr(dataPtr, sizeof(uint32_t))) {
                    float* floatValue = reinterpret_cast<float*>(*dataPtr);
                    if (floatValue && !IsBadReadPtr(floatValue, sizeof(float))) {
                        ss << "  Value: " << *floatValue << std::endl;
                    }
                }

                // Log the first 32 bytes of the structure as well
                for (int i = 0; i < 8; i++) {
                    uint32_t* intPtr = reinterpret_cast<uint32_t*>(*param_1 + i * 4);
                    if (!IsBadReadPtr(intPtr, sizeof(uint32_t))) {
                        ss << "  Offset " << i * 4 << ": 0x" << std::hex << *intPtr << std::dec << std::endl;
                    }
                }
                Log(ss.str());
            }
        }
        else {
            Log("Error: original_FUN_00c12830 is null");
            return;
        }

        Log("Exiting HookedFUN_00c12830");
    }
    catch (const SEHException& e) {
        Log(std::string("SEH Exception in HookedFUN_00c12830: ") + e.what());
    }
    catch (const std::exception& e) {
        Log(std::string("C++ Exception in HookedFUN_00c12830: ") + e.what());
    }
    catch (...) {
        Log("Unknown exception in HookedFUN_00c12830");
    }
}

void __fastcall HookedFUN_00c128c0(int param_1) {
    Log("Entering FUN_00c128c0");

    Log("param_1 address: 0x" + IntToHexString(param_1));

    // Call the original function
    original_FUN_00c128c0(param_1);

    // Examine memory at param_1
    std::stringstream ss;
    ss << "Interesting values at param_1:" << std::endl;
    int interestingOffsets[] = { 0x170, 0x1ac, 0x254, 0x34c, 0x4a4, 0x4d4, 0x598, 0x6b4, 0x6c0, 0x700, 0x750, 0x784, 0x7a4, 0x994, 0xa18, 0xa1c };
    for (int offset : interestingOffsets) {
        float* floatPtr = reinterpret_cast<float*>(param_1 + offset);
        if (!IsBadReadPtr(floatPtr, sizeof(float))) {
            ss << "  Offset 0x" << std::hex << offset << ": " << std::dec << *floatPtr << std::endl;
        }
    }
    Log(ss.str());

    Log("Exiting FUN_00c128c0");
}
void __stdcall HookedFUN_007d02e0(int* param_1_00, float* param_2, const wchar_t* param_3) {
    try {
        Log("Entering FUN_007d02e0");

        std::wstring configKey(param_3);
        std::string narrowConfigKey = WideToUTF8(configKey);

        std::stringstream ss;
        ss << "FUN_007d02e0 called with key: " << narrowConfigKey;
        if (param_2 && !IsBadReadPtr(param_2, sizeof(float))) {
            ss << ", value: " << *param_2;
        }
        else {
            ss << ", value: <invalid pointer>";
        }
        Log(ss.str());

        // Log the memory around param_2
        ss.str("");
        ss << "Memory around param_2:" << std::endl;
        for (int i = -16; i <= 16; i += 4) {
            float* floatPtr = reinterpret_cast<float*>(reinterpret_cast<char*>(param_2) + i);
            if (floatPtr && !IsBadReadPtr(floatPtr, sizeof(float))) {
                ss << "  Offset " << i << ": " << *floatPtr << std::endl;
            }
            else {
                ss << "  Offset " << i << ": <invalid memory>" << std::endl;
            }
        }
        Log(ss.str());

        original_FUN_007d02e0(param_1_00, param_2, param_3);

        Log("Exiting FUN_007d02e0");
    }
    catch (const std::exception& e) {
        Log(std::string("Exception in HookedFUN_007d02e0: ") + e.what());
    }
    catch (...) {
        Log("Unknown exception in HookedFUN_007d02e0");
    }
}

void AnalyzeAndLogConfigs()
{
    std::vector<std::pair<std::string, std::pair<std::string, uintptr_t>>> sortedConfigs;
    {
        std::lock_guard<std::mutex> lock(configMutex);
        sortedConfigs.assign(uniqueConfigs.begin(), uniqueConfigs.end());
    }

    std::sort(sortedConfigs.begin(), sortedConfigs.end(),
        [](const auto& a, const auto& b) { return a.first < b.first; });

    std::stringstream ss;
    ss << "Sorted Configuration Keys and Values:" << std::endl;
    ss << std::setw(40) << std::left << "Key" << std::setw(30) << "Value" << "Address" << std::endl;
    ss << std::string(80, '-') << std::endl;

    for (const auto& [key, value_address] : sortedConfigs)
    {
        ss << std::setw(40) << std::left << key
            << std::setw(30) << value_address.first
            << "0x" << std::hex << std::setw(8) << std::setfill('0') << value_address.second << std::dec << std::setfill(' ') << std::endl;
    }

    Log(ss.str());
}

// idk why I did it this way
void TriggerGameConfigAnalysis() {
    GameConfig::Instance().AnalyzeAndSortConfigs();
    Log("Game config analysis and sorting completed");
}


void* __fastcall HookedFUN_006ef780(void* param_1) {
    Log("Entering HookedFUN_006ef780");
    void* result = nullptr;
    if (original_FUN_006ef780) {
        try {
            result = original_FUN_006ef780(param_1);
            Log("FUN_006ef780 called successfully, returned: 0x" + std::to_string(reinterpret_cast<DWORD>(result)));

            AnalyzeBitfield();

            // Log the first few bytes of the returned structure
            unsigned char* resultBytes = static_cast<unsigned char*>(result);
            std::ostringstream bytesStream;
            bytesStream << "First 16 bytes of result: ";
            for (int i = 0; i < 16; i++) {
                bytesStream << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(resultBytes[i]) << " ";
            }
            Log(bytesStream.str());

            // Log the values of all toggles
            std::stringstream ss;
            ss << "Toggle values:" << std::endl;
            for (const auto& [type, info] : toggles) {
                bool value = ReadBoolValue(info.offset);
                ss << "  " << std::setw(30) << std::left << info.name << ": "
                    << (value ? "true" : "false") << " (0x" << std::hex
                    << static_cast<int>(ReadValue<uint8_t>(info.offset)) << ")" << std::endl;
            }
            Log(ss.str());
        }
        catch (...) {
            Log("Exception occurred in HookedFUN_006ef780");
        }
    }
    else {
        Log("Error: original_FUN_006ef780 is null");
    }
    Log("Exiting HookedFUN_006ef780");
    return result;
}

HRESULT __stdcall HookedEndScene(LPDIRECT3DDEVICE9 pDevice) {
    if (!IsGUIInitialized()) {
        InitializeGUI(pDevice);
    }

    RenderGUI(pDevice);

    return trampoline_EndScene(pDevice);
}
