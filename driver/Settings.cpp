#include "pch.h"
#include "Settings.h"
#include "Logger.h"

#include <windows.h>

#define SETTINGS_FILE "C:\\Users\\Public\\dpdu_settings.ini"

bool Settings::DisableTesterpresent = false;

void Settings::Load()
{
    DisableTesterpresent = GetPrivateProfileInt("Settings", "DisableTesterpresent", 0, SETTINGS_FILE);
    LOGGER.logInfo("Settings", "DisableTesterpresent %d", DisableTesterpresent);
}

