// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "dpdu.h"
#include "version.h"
#include "Logger.h"
#include "Settings.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        LOGGER.logInfo("VERSION", "dpdu-passthru version: %s", BUILD_VERSION);
        Settings::Load();
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }

    return TRUE;
}

