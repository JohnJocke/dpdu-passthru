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

#include "pch.h"
#include "Logger.h"

void Logger::logInfo(std::string method, const char* fmt, ...) {
	va_list fmtargs;
	va_start(fmtargs, fmt);
	writeToFile("[INFO ] " + method + " - " + argFormatToString(fmt, &fmtargs));
	va_end(fmtargs);
}

void Logger::logWarn(std::string method, const char* fmt, ...) {
	va_list fmtargs;
	va_start(fmtargs, fmt);
	writeToFile("[WARN ] " + method + " - " + argFormatToString(fmt, &fmtargs));
	va_end(fmtargs);
}

void Logger::logError(std::string method, const char* fmt, ...) {
	va_list fmtargs;
	va_start(fmtargs, fmt);
	writeToFile("[ERROR] " + method + " - " + argFormatToString(fmt, &fmtargs));
	va_end(fmtargs);
}

void Logger::logDebug(std::string method, const char* fmt, ...) {
	va_list fmtargs;
	va_start(fmtargs, fmt);
	writeToFile("[DEBUG] " + method + " - " + argFormatToString(fmt, &fmtargs));
	va_end(fmtargs);
}

std::string Logger::argFormatToString(const char* fmt, va_list* args) {
	char buffer[4096] = { 0x00 };
	int rc = vsnprintf_s(buffer, sizeof(buffer), fmt, *args);
	return std::string(buffer);
}

void Logger::writeToFile(std::string message) {
	char time[64] = { 0x00 };
	getTimestamp(time, sizeof(time));
	std::ofstream handle;
	this->mutex.lock();
	try {
		handle.open(LOG_FILE, std::ios_base::app);
		handle << time << message << "\n" << std::flush;
		handle.close();
	}
	catch (std::ofstream::failure e) {
		//TODO handle error
	}
	this->mutex.unlock();
}

std::string Logger::bytesToString(uint8_t* bytes, unsigned long len) {
	std::string ret = "";
	LOGGER.logDebug("LB", "%lu bytes", len);
	char buf[4] = { 0x00 };
	for (int i = 0; i < len; i++) {
		sprintf_s(buf, "%02X ", bytes[i]);
		ret += buf;
	}
	return ret;
}

void Logger::getTimestamp(char* buffer, size_t bufferSize)
{
	SYSTEMTIME utc, localTime;
	GetSystemTime(&utc);  // Get UTC time
	SystemTimeToTzSpecificLocalTime(NULL, &utc, &localTime);  // Convert to local times

	// Format timestamp in format: YYYY-MM-DD HH:MM:SS.sss
	snprintf(buffer, bufferSize, "%04d-%02d-%02d %02d:%02d:%02d.%03d ",
		localTime.wYear, localTime.wMonth, localTime.wDay,
		localTime.wHour, localTime.wMinute, localTime.wSecond, localTime.wMilliseconds);
}

Logger LOGGER;