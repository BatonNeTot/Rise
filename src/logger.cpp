//☀Rise☀
#include "Rise/logger.h"

#include <debugapi.h>

namespace Rise {

	void Logger::Log(Logger::Level level, const std::string& message) {
		if (level > _level) {
			return;
		}
		LogImpl(message + '\n');
		if (level > Level::Off && level <= Level::Warning) {
			__debugbreak();
		}
	}

	void Logger::LogImpl(const std::string& message) {
		OutputDebugString(message.c_str());
	}

}
