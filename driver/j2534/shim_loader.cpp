/*
**
** Copyright (C) 2009 Drew Technologies Inc.
** Author: Joey Oravec <joravec@drewtech.com>
**
** This library is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation, either version 3 of the License, or (at
** your option) any later version.
**
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public
** License along with this library; if not, <http://www.gnu.org/licenses/>.
**
*/

#include <afxmt.h>
//#include "../pch.h"

#include "j2534_v0404.h"
//#include "SelectionBox.h"
//#include "shim_debug.h"
#include "shim_loader.h"
//#include "shim_output.h"

// Pointers to J2534 API functions in the loaded library
PTOPEN _PassThruOpen = 0;
PTCLOSE _PassThruClose = 0;
PTCONNECT _PassThruConnect = 0;
PTDISCONNECT _PassThruDisconnect = 0;
PTREADMSGS _PassThruReadMsgs = 0;
PTWRITEMSGS _PassThruWriteMsgs = 0;
PTSTARTPERIODICMSG _PassThruStartPeriodicMsg = 0;
PTSTOPPERIODICMSG _PassThruStopPeriodicMsg = 0;
PTSTARTMSGFILTER _PassThruStartMsgFilter = 0;
PTSTOPMSGFILTER _PassThruStopMsgFilter = 0;
PTSETPROGRAMMINGVOLTAGE _PassThruSetProgrammingVoltage = 0;
PTREADVERSION _PassThruReadVersion = 0;
PTGETLASTERROR _PassThruGetLastError = 0;
PTIOCTL _PassThruIoctl = 0;

static HINSTANCE hDLL = NULL;

static bool fLibLoaded = false;
static LARGE_INTEGER ticksPerSecond;
static LARGE_INTEGER tick;
static CRITICAL_SECTION mAutoLock;

// Vista-forward has a great function InitOnceExecuteOnce() to thread-safe execute
// a callback exactly once, but we want to support Windows XP. Instead we'll guard
// with a globally initialized CCriticalSection.
static CCriticalSection CritSectionPerformanceCounter;
static bool fPerformanceCounterInitialized = false;
static CCriticalSection CritSectionAutoLock;
static bool fAutoLockInitialized = false;

auto_lock::auto_lock()
{
	// ONCE -- the first time somebody creates an autolock we need to initialize the mutex
	CritSectionAutoLock.Lock();
	if (! fAutoLockInitialized)
	{
		InitializeCriticalSection(&mAutoLock);
		fAutoLockInitialized = true;
	}
	CritSectionAutoLock.Unlock();

	if (! TryEnterCriticalSection(&mAutoLock))
	{
		//dtDebug(_T("Multi-threading error"));
		EnterCriticalSection(&mAutoLock);
	}
}

auto_lock::~auto_lock()
{
	LeaveCriticalSection(&mAutoLock);
}

// Find all J2534 v04.04 interfaces listed in the registry
void shim_enumPassThruInterfaces(std::set<cPassThruInfo> &registryList)
{
	HKEY hKey1,hKey2,hKey3;
	FILETIME FTime;
	long hKey2RetVal;
	DWORD VendorIndex;
	DWORD KeyType;
	DWORD KeySize;

	registryList.clear();

	// Open HKLM/Software
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software"), 0, KEY_READ, &hKey1) != ERROR_SUCCESS)
	{
		//strcpy_s(J2534BoilerplateErrorResult, sizeof(J2534BoilerplateErrorResult), "Can't open HKEY_LOCAL_MACHINE->Software key.");
		return;
	}

	// Open HKLM/Software/PassThruSupport.04.04
	if (RegOpenKeyEx(hKey1, _T("PassThruSupport.04.04"), 0, KEY_READ, &hKey2) != ERROR_SUCCESS)
	{
		//strcpy_s(J2534BoilerplateErrorResult, sizeof(J2534BoilerplateErrorResult), "Can't open HKEY_LOCAL_MACHINE->..->PassThruSupport.04.04 key");
		RegCloseKey(hKey1);
		return;
	}
	RegCloseKey(hKey1);

	// Determine the maximum subkey length for HKLM/Software/PassThruSupport.04.04/*
	DWORD lMaxSubKeyLen;
	RegQueryInfoKey(hKey2, NULL, NULL, NULL, NULL, &lMaxSubKeyLen, NULL, NULL, NULL, NULL, NULL, NULL);

	// Allocate a buffer large enough to hold that name
	TCHAR * KeyValue = new TCHAR[lMaxSubKeyLen+1];

	// Iterate through HKLM/Software/PassThruSupport.04.04/*
	VendorIndex = 0;
	do
	{
		// Get the name of HKLM/Software/PassThruSupport.04.04/VendorDevice[i]
		KeySize = lMaxSubKeyLen+1;
		hKey2RetVal = RegEnumKeyEx(hKey2, VendorIndex++, KeyValue, &KeySize, NULL, NULL, NULL, &FTime);
		if (hKey2RetVal != ERROR_SUCCESS)
		{
			continue;
		}

#ifdef DREWTECHONLY
		// Check to see if it is Drew Tech
		if (strncmp("Drew Tech", (char *)KeyValue, 9) != 0)
		{
			continue;
		}
#endif

		// Open HKLM/Software/PassThruSupport.04.04/Vendor[i]
		if (RegOpenKeyEx(hKey2, KeyValue, 0, KEY_READ, &hKey3) == ERROR_SUCCESS)
		{
			tstring strVendor, strName, strFunctionLibrary, strConfigApplication;
			LSTATUS retval;

			// Determine the maximum value length for HKLM/Software/PassThruSupport.04.04/VendorDevice[i]/*
			DWORD lMaxValueLen;
			retval = RegQueryInfoKey(hKey3, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &lMaxValueLen, NULL, NULL);

			// Allocate a buffer large enough to hold that name
			TCHAR * KeyValue = new TCHAR[lMaxValueLen+1];

			// Query HKLM/Software/PassThruSupport.04.04/VendorDevice[i]/Vendor
			KeySize = lMaxValueLen+1;
			retval = RegQueryValueEx(hKey3, _T("Vendor"), 0, &KeyType, (LPBYTE) KeyValue, &KeySize);
			if (retval == ERROR_SUCCESS)
			{
				strVendor = KeyValue;
			}

			// Query HKLM/Software/PassThruSupport.04.04/VendorDevice[i]/Name
			KeySize = lMaxValueLen+1;
			retval = RegQueryValueEx(hKey3, _T("Name"), 0, &KeyType, (LPBYTE) KeyValue, &KeySize);
			if (retval == ERROR_SUCCESS)
			{
				strName = KeyValue;
			}

			// Read HKLM/Software/PassThruSupport.04.04/VendorDevice[i]/FunctionLibrary
			KeySize = lMaxValueLen+1;
			retval = RegQueryValueEx(hKey3, _T("FunctionLibrary"), 0, &KeyType, (LPBYTE) KeyValue, &KeySize);
			if (retval == ERROR_SUCCESS)
			{
				strFunctionLibrary = KeyValue;
			}

			// Read HKLM/Software/PassThruSupport.04.04/VendorDevice[i]/ConfigApplication
			KeySize = lMaxValueLen+1;
			retval = RegQueryValueEx(hKey3, _T("ConfigApplication"), 0, &KeyType, (LPBYTE) KeyValue, &KeySize);
			if (retval == ERROR_SUCCESS)
			{
				strConfigApplication = KeyValue;
			}

			RegCloseKey(hKey3);
			delete[] KeyValue;

			// If everything was successful then add it to the list
			cPassThruInfo registryEntry(strVendor, strName, strFunctionLibrary, strConfigApplication);
			registryList.insert(registryEntry);
		}
	} while (hKey2RetVal == ERROR_SUCCESS);

	RegCloseKey(hKey2);
	delete[] KeyValue;
}

double GetTimeSinceInit()
{
	LARGE_INTEGER tock;
    double time;

	// ONCE -- the first time somebody gets a timestamp set the timer to 0.000s
	CritSectionPerformanceCounter.Lock();
	if (! fPerformanceCounterInitialized)
	{
		QueryPerformanceFrequency(&ticksPerSecond);
		QueryPerformanceCounter(&tick);
		fPerformanceCounterInitialized = true;
	}
	CritSectionPerformanceCounter.Unlock();

	QueryPerformanceCounter(&tock);
	time = (double)(tock.QuadPart-tick.QuadPart)/(double)ticksPerSecond.QuadPart;
	return time;
}

bool shim_checkAndAutoload(void)
{
	// We're OK if a library is loaded
	if (fLibLoaded)
		return true;

	// Define ALLOW_POPUP if you want this function to continue by scaning the registry, presenting
	// a dialog, and allowing the user to pick a J2534 DLL. Leave it undefined if you want to force
	// the app to call PassThruLoadLibrary

#ifndef ALLOW_POPUP
	return false;
#endif
//
//	// Check the registry for J2534 interfaces
//	std::set<cPassThruInfo> interfaceList;
//	EnumPassThruInterfaces(interfaceList);
//
//	if (interfaceList.size() == 0)
//	{
//		// No interfaces listed in the registry? Failure!
//		return false;
//	}
//#if 0
//	// This would be a nice optimization, but then the user doesn't get to pick
//	// a log output destination?? That's bad. For now keep it disabled
//	else if (interfaceList.size() == 1)
//	{
//		// One interface? Pick it automatically! But 
//		std::set<cPassThruInfo>::iterator iInterface;
//		iInterface = interfaceList.begin();
//		LoadJ2534DLL(iInterface->FunctionLibrary.c_str());
//	
//		fLibLoaded = true;
//	}
//#endif
//	else
//	{
//		// Multiple interfaces? Popup a selection box!
//		INT_PTR retval;
//		CSelectionBox Dlg(interfaceList);
//
//		retval = Dlg.DoModal();
//		if (retval == IDCANCEL)
//		{
//			return false;
//		}
//
//		cPassThruInfo * tmp = Dlg.GetSelectedPassThru();
//
//		bool fSuccess;
//		fSuccess = shim_loadLibrary(tmp->FunctionLibrary.c_str());
//		if (! fSuccess)
//		{
//			//shim_setInternalError(_T("Failed to open '%s'"), tmp->FunctionLibrary.c_str());
//			//dbug_printretval(ERR_FAILED);
//			return false;
//		}
//		fLibLoaded = true;
//
//		// The user specified a debug output file in the dialog. Write any buffered text to this file
//		// and start using it from now on
//		//shim_writeLogfile(Dlg.GetDebugFilename(), true);
//
//		return true;
//	}
}

bool shim_loadLibrary(LPCTSTR szDLL)
{
	// Can't load a library if the string is NULL
	if (szDLL == NULL)
	{
		return false;
	}

	// Can't load a library if there's one currently loaded
	if (fLibLoaded)
	{
		return false;
	}

	hDLL = LoadLibrary(szDLL);
	if (hDLL == NULL)
	{
		// Try to get the error text
		// Set the internal error text based on the win32 message
		return false;
	}

	fLibLoaded = true;

	_PassThruOpen = (PTOPEN)GetProcAddress(hDLL, "PassThruOpen");
	_PassThruClose = (PTCLOSE)GetProcAddress(hDLL, "PassThruClose");
	_PassThruConnect = (PTCONNECT)GetProcAddress(hDLL, "PassThruConnect");
	_PassThruDisconnect = (PTDISCONNECT)GetProcAddress(hDLL, "PassThruDisconnect");
	_PassThruReadMsgs = (PTREADMSGS)GetProcAddress(hDLL, "PassThruReadMsgs");
	_PassThruWriteMsgs = (PTWRITEMSGS)GetProcAddress(hDLL, "PassThruWriteMsgs");
	_PassThruStartPeriodicMsg = (PTSTARTPERIODICMSG)GetProcAddress(hDLL, "PassThruStartPeriodicMsg");
	_PassThruStopPeriodicMsg = (PTSTOPPERIODICMSG)GetProcAddress(hDLL, "PassThruStopPeriodicMsg");
	_PassThruStartMsgFilter = (PTSTARTMSGFILTER)GetProcAddress(hDLL, "PassThruStartMsgFilter");
	_PassThruStopMsgFilter = (PTSTOPMSGFILTER)GetProcAddress(hDLL, "PassThruStopMsgFilter");
	_PassThruSetProgrammingVoltage = (PTSETPROGRAMMINGVOLTAGE)GetProcAddress(hDLL, "PassThruSetProgrammingVoltage");
	_PassThruReadVersion = (PTREADVERSION)GetProcAddress(hDLL, "PassThruReadVersion");
	_PassThruGetLastError = (PTGETLASTERROR)GetProcAddress(hDLL, "PassThruGetLastError");
	_PassThruIoctl = (PTIOCTL)GetProcAddress(hDLL, "PassThruIoctl");

	return true;
}

void shim_unloadLibrary()
{
	// Can't unload a library if there's nothing loaded
	if (! fLibLoaded)
		return;

	fLibLoaded = false;

	// Invalidate the function pointers
	_PassThruOpen = NULL;
	_PassThruClose = NULL;
	_PassThruConnect = NULL;
	_PassThruDisconnect = NULL;
	_PassThruReadMsgs = NULL;
	_PassThruWriteMsgs = NULL;
	_PassThruStartPeriodicMsg = NULL;
	_PassThruStopPeriodicMsg = NULL;
	_PassThruStartMsgFilter = NULL;
	_PassThruStopMsgFilter = NULL;
	_PassThruSetProgrammingVoltage = NULL;
	_PassThruReadVersion = NULL;
	_PassThruGetLastError = NULL;
	_PassThruIoctl = NULL;

	BOOL fSuccess;
	fSuccess = FreeLibrary(hDLL);
	if (! fSuccess)
	{
		// Try to get the error text
		// Set the internal error text based on the win32 message
	}
}

bool shim_hasLibraryLoaded()
{
	return fLibLoaded;
}