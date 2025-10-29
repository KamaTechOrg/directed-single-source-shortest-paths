#pragma once
#include <iostream>
#include <sstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <string>

namespace logsys {
    enum class Level { DEBUG, INFO, ERROR };
    inline Level& current_level() { static Level L = Level::DEBUG; return L; }

    inline std::mutex& mu() { static std::mutex m; return m; }

    inline std::string ts() {
        using namespace std::chrono;
        auto now = system_clock::now();
        auto t = system_clock::to_time_t(now);
        auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
        std::tm tm_buf{};
        #if defined(_WIN32) || defined(_WIN64)
           +localtime_s(&tm_buf, &t);     // בטוח בחלונות
        #else
           + localtime_r(&t, &tm_buf);     // בטוח ב-POSIX (לינוקס/מק)
        #endif
            std::ostringstream o;
        o << std::put_time(&tm_buf, "%H:%M:%S")
            << '.' << std::setw(3) << std::setfill('0') << ms.count();
        return o.str();
    }

    class Logger {
    public:
        explicit Logger(Level lvl) : lvl_(lvl) {}
        ~Logger() {
            std::lock_guard<std::mutex> lock(mu());
            std::ostream& out = (lvl_ == Level::ERROR ? std::cerr : std::cout);
            out << "[" << ts() << "] " << tag(lvl_) << ' ' << ss_.str() << std::endl;
        }
        template <class T> Logger& operator<<(const T& v) { ss_ << v; return *this; }
    private:
        Level lvl_;
        std::ostringstream ss_;
        static const char* tag(Level L) {
            switch (L) {
            case Level::DEBUG: return "[DEBUG]";
            case Level::INFO:  return "[INFO ]";
            case Level::ERROR: return "[ERROR]";
            }
            return "[UNKWN]";
        }
    };
} // namespace logsys

#define LOG_DEBUG() if (logsys::current_level() <= logsys::Level::DEBUG) logsys::Logger(logsys::Level::DEBUG)
#define LOG_INFO()  if (logsys::current_level() <= logsys::Level::INFO ) logsys::Logger(logsys::Level::INFO)
#define LOG_ERROR() logsys::Logger(logsys::Level::ERROR)
