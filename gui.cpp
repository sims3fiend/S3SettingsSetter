#include "gui.h"
#include "renderer.h"
#include "utils.h"
#include "imgui.h"
#include "hooks.h"
#include "game_config.h"
#include "script_settings.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx9.h"


WNDPROC oWndProc = NULL;
HWND hWnd = NULL;
bool imguiInitialized = false;
bool showImGui = false;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool IsGUIInitialized() {
    return imguiInitialized;
}

void LoadRenderSettings() {
    ScriptSettings& settings = ScriptSettings::Instance();
    void* rendererAddress = GetRendererStructureAddress();

    if (!rendererAddress) {
        Log("Renderer structure address not available. Cannot apply render settings.");
        return;
    }

    for (const auto& [type, info] : toggles) {
        auto value = settings.GetSettingValue("RenderSettings", info.name, ReadBoolValue(info.offset));
        if (std::holds_alternative<bool>(value)) {
            WriteBoolValue(info.offset, std::get<bool>(value));
            Log("Applied " + std::string(info.name) + " setting: " + (std::get<bool>(value) ? "true" : "false"));
        }
    }
}

void SaveRenderSetting(const std::string& name, bool value) {
    ScriptSettings::Instance().SetSettingValue("RenderSettings", name, value);
    ScriptSettings::Instance().SaveToFile();
}

void InitializeGUI(LPDIRECT3DDEVICE9 pDevice) {
    D3DDEVICE_CREATION_PARAMETERS params;
    pDevice->GetCreationParameters(&params);
    hWnd = params.hFocusWindow;

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls

    oWndProc = (WNDPROC)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)WndProc);

    ImGui_ImplWin32_Init(hWnd);
    ImGui_ImplDX9_Init(pDevice);

    LoadRenderSettings();  // Load and apply render settings

    imguiInitialized = true;
}

void RenderGUI(LPDIRECT3DDEVICE9 pDevice) {
    if (showImGui) {
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        DrawImGuiInterface();

        ImGui::EndFrame();
        ImGui::Render();
        ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
    }
}

void CleanupGUI() {
    Log("Cleaning up GUI");
    if (imguiInitialized) {
        //        ImGui_ImplDX9_Shutdown(); //this makes us hang idk why :)

        Log("Shutting down ImGui Win32 implementation");
        ImGui_ImplWin32_Shutdown();
        Log("ImGui Win32 shutdown completed");


        if (oWndProc && hWnd) {
            SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)oWndProc);
        }
        imguiInitialized = false;
        showImGui = false;
        hWnd = NULL;
        oWndProc = NULL;
    }
    Log("GUI cleanup completed");
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (showImGui) {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
            return true;

        // Block all input when ImGui is visible, except for the toggle key
        switch (msg) {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONDBLCLK:
        case WM_MOUSEWHEEL:
        case WM_MOUSEMOVE:
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_CHAR:
            if (msg == WM_KEYDOWN && wParam == VK_INSERT) {
                showImGui = !showImGui;
                Log("Toggled ImGui visibility: " + std::to_string(showImGui));
            }
            return true;
        }
    }
    else {
        // When ImGui is hidden, only check for the toggle key
        if (msg == WM_KEYDOWN && wParam == VK_INSERT) {
            showImGui = !showImGui;
            Log("Toggled ImGui visibility: " + std::to_string(showImGui));
            return true;
        }
    }

    return CallWindowProc(oWndProc, hWnd, msg, wParam, lParam);
}

void DrawImGuiInterface() {
    void* rendererAddress = GetRendererStructureAddress();
    if (!rendererAddress) {
        ImGui::Begin("Renderer Not Initialized");
        ImGui::Text("Waiting for renderer structure address...");
        ImGui::End();
        return;
    }

    ImGui::Begin("Graphics Tweaker");

    ImGui::Text("Press Insert to toggle UI visibility");
    ImGui::Text("Renderer Structure Address: 0x%X", reinterpret_cast<uintptr_t>(rendererAddress));
    ImGui::Separator();

    if (ImGui::BeginTabBar("MainTabBar")) {
        if (ImGui::BeginTabItem("Render Toggles")) {
            for (const auto& [type, info] : toggles) {
                uint8_t byteValue = ReadValue<uint8_t>(info.offset);
                bool isOn = byteValue & 1;
                if (ImGui::Checkbox(info.name, &isOn)) {
                    WriteBoolValue(info.offset, isOn);
                    SaveRenderSetting(info.name, isOn);
                    Log(std::string(info.name) + " toggled. New value: " + (isOn ? "true" : "false"));
                }
                ImGui::SameLine();
                ImGui::Text("(0x%02X)", byteValue);
            }
            if (ImGui::Button("Dump Renderer Memory")) {
                DumpRendererMemory();
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Game Values")) {
            for (const auto& [type, info] : values) {
                int currentValue = ReadIntValue(type);
                if (ImGui::SliderInt(info.name, &currentValue, info.minValue, info.maxValue)) {
                    WriteIntValue(type, currentValue);
                    ScriptSettings::Instance().SetSettingValue("GameValues", info.name, std::to_string(currentValue));
                    ScriptSettings::Instance().SaveToFile();
                    Log(std::string(info.name) + " changed. New value: " + std::to_string(currentValue));
                }
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Game Config")) {
            const auto& configs = GameConfig::Instance().GetAllConfigs();
            ImGui::Text("These are config settings but pulled directly from the game.\nIf you see a value here that doesn't line up with your settings files, idk.\nMore coming (tunables) soon...maybe.");

            if (ImGui::CollapsingHeader("Boolean Settings")) {
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("These are from a separate function to the string ones, but do the same thing");
                }
                for (const auto& [key, configEntry] : configs) {
                    if (key.find("CfgFull") != std::string::npos) {
                        continue;
                    }
                    if (std::holds_alternative<bool>(configEntry.value)) {
                        ImGui::PushID(key.c_str());

                        bool isOverridden = ScriptSettings::Instance().GetAllSettings().count("GameConfig." + key) > 0;
                        bool override = isOverridden;
                        if (ImGui::Checkbox("##Override", &override)) {
                            if (override) {
                                ScriptSettings::Instance().AddOrUpdateSetting("GameConfig", key, configEntry.value);
                            }
                            else {
                                ScriptSettings::Instance().RemoveSetting("GameConfig", key);
                            }
                            Log(key + " override changed. New status: " + (override ? "enabled" : "disabled"));
                        }

                        ImGui::SameLine();

                        bool currentValue = std::get<bool>(configEntry.value);
                        if (isOverridden) {
                            auto overriddenValue = ScriptSettings::Instance().GetSettingValue("GameConfig", key, currentValue);
                            if (std::holds_alternative<bool>(overriddenValue)) {
                                currentValue = std::get<bool>(overriddenValue);
                            }
                        }

                        const char* items[] = { "false", "true" };
                        int currentItem = currentValue ? 1 : 0;
                        if (ImGui::Combo(key.c_str(), &currentItem, items, IM_ARRAYSIZE(items))) {
                            bool newValue = (currentItem == 1);
                            if (isOverridden) {
                                ScriptSettings::Instance().SetSettingValue("GameConfig", key, newValue);
                            }
                            else {
                                GameConfig::Instance().SetConfigValue(key, newValue);
                            }
                            Log(key + " changed. New value: " + (newValue ? "true" : "false"));
                        }

                        ImGui::PopID();
                    }
                }
            }

            if (ImGui::CollapsingHeader("String Settings")) {
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("These are from a separate function to the boolean ones, but do the same thing");
                }
                for (const auto& [key, configEntry] : configs) {
                    if (key.find("CfgFull") != std::string::npos) {
                        continue;
                    }
                    if (std::holds_alternative<std::string>(configEntry.value)) {
                        ImGui::PushID(key.c_str());

                        bool isOverridden = ScriptSettings::Instance().GetAllSettings().count("GameConfig." + key) > 0;
                        bool override = isOverridden;
                        if (ImGui::Checkbox("##Override", &override)) {
                            if (override) {
                                ScriptSettings::Instance().AddOrUpdateSetting("GameConfig", key, configEntry.value);
                            }
                            else {
                                ScriptSettings::Instance().RemoveSetting("GameConfig", key);
                            }
                            Log(key + " override changed. New status: " + (override ? "enabled" : "disabled"));
                        }

                        ImGui::SameLine();

                        std::string currentValue = std::get<std::string>(configEntry.value);
                        if (isOverridden) {
                            auto overriddenValue = ScriptSettings::Instance().GetSettingValue("GameConfig", key, currentValue);
                            if (std::holds_alternative<std::string>(overriddenValue)) {
                                currentValue = std::get<std::string>(overriddenValue);
                            }
                        }
                        char buffer[256];
                        strcpy_s(buffer, currentValue.c_str());
                        if (ImGui::InputText(key.c_str(), buffer, sizeof(buffer))) {
                            if (isOverridden) {
                                ScriptSettings::Instance().SetSettingValue("GameConfig", key, std::string(buffer));
                            }
                            else {
                                GameConfig::Instance().SetConfigValue(key, std::string(buffer));
                            }
                            Log(key + " changed. New value: " + buffer);
                        }

                        ImGui::PopID();
                    }
                }
            }

            if (ImGui::Button("Save Config")) {
                GameConfig::Instance().SaveToFile();
            }
            ImGui::SameLine();
            if (ImGui::Button("Load Config")) {
                GameConfig::Instance().LoadFromFile();
            }
            ImGui::SameLine();
            if (ImGui::Button("Save Overrides")) {
                ScriptSettings::Instance().SaveToFile();
            }
            ImGui::SameLine();
            if (ImGui::Button("Load Overrides")) {
                ScriptSettings::Instance().LoadFromFile();
            }
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}