#include "pch.h"
#include "Settings.h"
#include "Logger.h"

#include <windows.h>

#define SETTINGS_FILE "C:\\Users\\Public\\dpdu_settings.ini"

bool Settings::DisableTesterpresent = false;
bool Settings::FixTesterpresentDestination = false;
bool Settings::AutoRestartComm = false;

void Settings::Load()
{
    DisableTesterpresent = GetPrivateProfileInt("Settings", "DisableTesterpresent", 0, SETTINGS_FILE);
    FixTesterpresentDestination = GetPrivateProfileInt("Settings", "FixTesterpresentDestination", 0, SETTINGS_FILE);
    AutoRestartComm = GetPrivateProfileInt("Settings", "AutoRestartComm", 0, SETTINGS_FILE);

    LOGGER.logInfo("Settings", "DisableTesterpresent: %d", DisableTesterpresent);
    LOGGER.logInfo("Settings", "FixTesterpresentDestination: %d", FixTesterpresentDestination);
    LOGGER.logInfo("Settings", "AutoRestartComm: %d", AutoRestartComm);
}

