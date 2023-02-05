//☀Rise☀
#ifndef logger_h
#define logger_h

#include <string>

namespace Rise {
    
    class Logger {
    public:

        enum class Level {
            Off = 0,
            Fatal,
            Error,
            Warning,
            Info,
            Debug,
            Trace
        };

        Logger() = default;
        Logger(Level level) : _level(level) {};
        ~Logger() = default;

        void setLevel(Level level) {
            _level = level;
        }

        void Log(Level level, const std::string& message);

        void Fatal(const std::string& message) { Log(Level::Fatal, message); }
        void Error(const std::string& message) { Log(Level::Error, message); }
        void Warning(const std::string& message) { Log(Level::Warning, message); }
        void Info(const std::string& message) { Log(Level::Info, message); }
        void Debug(const std::string& message) { Log(Level::Debug, message); }
        void Trace(const std::string& message) { Log(Level::Trace, message); }

    private:

        void LogImpl(const std::string& message);

        Level _level = Level::Off;

    };

}

#endif /* logger_h */
