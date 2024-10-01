#pragma once
#include <string>

void InitializeLogging();
void Log(const std::string& message);
void CleanupLogging();