#include "utils.h"
#include <fstream>
#include <iomanip>
#include <Windows.h>
#include <mutex>

std::ofstream logFile;

void InitializeLogging() {
    logFile.open("hook_log.txt", std::ios::trunc); //app for append
    if (!logFile.is_open()) {
        MessageBoxA(NULL, "Failed to open log file.", "Error", MB_ICONERROR);
    }
}

std::mutex logMutex;

void Log(const std::string& message) {
    std::lock_guard<std::mutex> lock(logMutex);
    if (logFile.is_open()) {
        SYSTEMTIME st;
        GetLocalTime(&st);
        logFile << std::setfill('0')
            << std::setw(2) << st.wHour << ":"
            << std::setw(2) << st.wMinute << ":"
            << std::setw(2) << st.wSecond << "."
            << std::setw(3) << st.wMilliseconds << " - "
            << message << std::endl;
        logFile.flush();
    }
}

void CleanupLogging() {
    Log("Cleaning up logging");
    if (logFile.is_open()) {
        Log("DLL detached, hooks removed");
        logFile.close();
    }
    Log("Logging cleanup completed");
}