/*
**
** Copyright (C) 2020 Ashcon Mohseninia
** Author: Ashcon Mohseninia <ashcon50@gmail.com>
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

#pragma once

#include <fstream>
#include <iostream>
#include <sstream>
#include <Windows.h>
#include <mutex>

#define LOG_FILE "C:\\Users\\Public\\dpdu_activity.log"

class Logger
{
private:
	std::mutex mutex;
	std::string argFormatToString(const char* fmt, va_list* args);
	void getTimestamp(char* buffer, size_t bufferSize);
public:
	std::string bytesToString(uint8_t* bytes, unsigned long len);
	void logInfo(std::string method, const char* fmt, ...);
	void logWarn(std::string method, const char* fmt, ...);
	void logError(std::string method, const char* fmt, ...);
	void logDebug(std::string method, const char* fmt, ...);
	void writeToFile(std::string message);
};

extern Logger LOGGER;

