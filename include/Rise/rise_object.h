//☀Rise☀
#ifndef rise_object_h
#define rise_object_h

#include "logger.h"

#include <string>

namespace Rise {

	class Core;

	class RiseObject {
	public:

		void Log(Logger::Level level, const std::string& message) const;

		void Fatal(const std::string& message) const { Log(Logger::Level::Fatal, message); }
		void Error(const std::string& message) const { Log(Logger::Level::Fatal, message); }
		void Warning(const std::string& message) const { Log(Logger::Level::Fatal, message); }
		void Info(const std::string& message) const { Log(Logger::Level::Fatal, message); }
		void Debug(const std::string& message) const { Log(Logger::Level::Fatal, message); }
		void Trace(const std::string& message) const { Log(Logger::Level::Fatal, message); }

	protected:

		RiseObject(Core* core) : _core(core) {}

	private:

		Core* _core;
	};

}

#endif /* rise_object_h */
