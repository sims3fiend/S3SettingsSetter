#pragma once
#include <Windows.h>
#include <d3d9.h>
#include "renderer.h"

bool InitializeHooks();
void CleanupHooks();
void* __fastcall HookedFUN_006ef780(void* param_1);
void TriggerGameSettingsAnalysis();
void AnalyzeAndLogConfigs();

class ConfigReadException : public std::exception {
public:
    ConfigReadException(const char* message) : msg_(message) {}
    const char* what() const noexcept override { return msg_; }
private:
    const char* msg_;
};

unsigned char __cdecl HookedGetConfigBoolWithKeyConstruction(int* param_1, unsigned char param_2);
HRESULT __stdcall HookedEndScene(LPDIRECT3DDEVICE9 pDevice);

typedef unsigned int(__thiscall* RetrieveConfigValue_t)(int param_1_00, short* param_2, int* param_3, void** param_4);
extern RetrieveConfigValue_t original_RetrieveConfigValue;

typedef wchar_t* (__cdecl* GetConfigValueAsString_t)(short* param_1, int* param_2, wchar_t* param_3);
extern GetConfigValueAsString_t original_GetConfigValueAsString;

unsigned int __fastcall HookedRetrieveConfigValue(int param_1_00, int dummy, short* param_2, int* param_3, void** param_4);
wchar_t* __cdecl HookedGetConfigValueAsString(short* param_1, int* param_2, wchar_t* param_3);

typedef DWORD(__thiscall* ParseConfigIni_t)(int param_1_00, wchar_t* param_2, DWORD param_3);
extern ParseConfigIni_t original_ParseConfigIni;
DWORD __fastcall HookedParseConfigIni(int param_1_00, int dummy, wchar_t* param_2, DWORD param_3);

void __stdcall ReadTunableSettings(int configHandle);
int __stdcall ReadFloatValue(int configHandle, const wchar_t* key, float* value);
typedef int(__thiscall* ReadConfigValue_t)(void* thisPtr, const wchar_t* key, float* value);

typedef DWORD(__thiscall* FUN_004eedf0_t)(int param_1_00, int param_2);
extern FUN_004eedf0_t original_FUN_004eedf0;

DWORD __fastcall HookedFUN_004eedf0(int param_1_00, int dummy, int param_2);

typedef unsigned int(__thiscall* FUN_007cff40_t)(int param_1_00, int* param_2);
extern FUN_007cff40_t original_FUN_007cff40;

unsigned int __fastcall HookedFUN_007cff40(int param_1_00, int dummy, int* param_2);

typedef void(__thiscall* FUN_006c3480_t)(void* param_1_00, int param_2);
extern FUN_006c3480_t original_FUN_006c3480;

void __fastcall HookedFUN_006c3480(void* param_1_00, int dummy, int param_2);

//FUN_00c12830
typedef void(__cdecl* FUN_00c12830_t)(wchar_t** param_1, const wchar_t* param_2, const wchar_t* param_3);
extern FUN_00c12830_t original_FUN_00c12830;

void __cdecl HookedFUN_00c12830(wchar_t** param_1, const wchar_t* param_2, const wchar_t* param_3);

//FUN_00c128c0
typedef void(__fastcall* FUN_00c128c0_t)(int param_1);
extern FUN_00c128c0_t original_FUN_00c128c0;

void __fastcall HookedFUN_00c128c0(int param_1);

//FUN_007d02e0
typedef void(__stdcall* FUN_007d02e0_t)(int* param_1_00, float* param_2, const wchar_t* param_3);
extern FUN_007d02e0_t original_FUN_007d02e0;

void __stdcall HookedFUN_007d02e0(int* param_1_00, float* param_2, const wchar_t* param_3);
