#ifdef ERROR
#undef ERROR
#endif

#pragma once
#include <iostream>
#include <mutex>
#include <chrono>
#include <ctime>

enum class LogLevel { INFO, WARN, ERROR, DEBUG };

class Logger {
private:
    static void log(LogLevel lvl, const std::string& msg) {
        static std::mutex mtx;
        std::lock_guard<std::mutex> lock(mtx);

        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);

        std::string lvl_str;
        switch (lvl) {
            case LogLevel::INFO: lvl_str = "INFO"; break;
            case LogLevel::WARN: lvl_str = "WARN"; break;
            case LogLevel::ERROR: lvl_str = "ERROR"; break;
            case LogLevel::DEBUG: lvl_str = "DEBUG"; break;
        }

        std::cout << "[" << std::string(std::ctime(&t)).substr(0, 19)
                  << "][" << lvl_str << "] "
                  << msg << std::endl;
    }
public:
    static void info(const std::string& msg) { log(LogLevel::INFO, msg); }
    static void warn(const std::string& msg) { log(LogLevel::WARN, msg); }
    static void error(const std::string& msg) { log(LogLevel::ERROR, msg); }
    static void debug(const std::string& msg) { log(LogLevel::DEBUG, msg); }
};
