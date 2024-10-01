#pragma once
#include <d3d9.h>
#include "renderer.h"

bool IsGUIInitialized();
void InitializeGUI(LPDIRECT3DDEVICE9 pDevice);
void RenderGUI(LPDIRECT3DDEVICE9 pDevice);
void CleanupGUI();
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
void DrawImGuiInterface();