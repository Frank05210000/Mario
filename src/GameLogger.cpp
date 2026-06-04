#include "GameLogger.hpp"

#include "Util/Logger.hpp"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

#include <spdlog/logger.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

std::string GameLogger::s_LogFilePath;

namespace {

std::string BuildTimestamp() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);

    std::tm localTime{};
#if defined(_WIN32)
    localtime_s(&localTime, &nowTime);
#else
    localtime_r(&nowTime, &localTime);
#endif

    std::ostringstream oss;
    oss << std::put_time(&localTime, "%Y-%m-%d_%H-%M-%S");
    return oss.str();
}

void InitConsoleOnlyLogger() {
    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto logger = std::make_shared<spdlog::logger>("OOPL_Mario", consoleSink);
    spdlog::set_default_logger(logger);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
    Util::Logger::SetLevel(Util::Logger::Level::DEBUG);
}

} // namespace

void GameLogger::Init() {
    if (!s_LogFilePath.empty()) {
        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
        Util::Logger::SetLevel(Util::Logger::Level::DEBUG);
        spdlog::flush_on(spdlog::level::info);
        LOG_INFO("Logger settings restored. Log file: {}", s_LogFilePath);
        return;
    }

    try {
#if defined(RESOURCE_DIR)
        const std::filesystem::path logDir =
            std::filesystem::path(RESOURCE_DIR).parent_path() / "logs";
#else
        const std::filesystem::path logDir = std::filesystem::current_path() / "logs";
#endif
        std::filesystem::create_directories(logDir);

        const std::filesystem::path logPath =
            logDir / ("mario_" + BuildTimestamp() + ".log");
        s_LogFilePath = logPath.string();

        std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
        sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(s_LogFilePath, false));

        auto logger = std::make_shared<spdlog::logger>("OOPL_Mario", sinks.begin(), sinks.end());
        spdlog::set_default_logger(logger);
        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
        Util::Logger::SetLevel(Util::Logger::Level::DEBUG);
        spdlog::flush_on(spdlog::level::info);

        LOG_INFO("Logger initialized. Log file: {}", s_LogFilePath);
    } catch (const std::exception& e) {
        s_LogFilePath.clear();
        InitConsoleOnlyLogger();
        LOG_WARN("Logger file output unavailable. Falling back to console-only logging: {}", e.what());
    }
}

void GameLogger::Shutdown() {
    LOG_INFO("Logger shutdown.");
    spdlog::shutdown();
}

const std::string& GameLogger::GetLogFilePath() {
    return s_LogFilePath;
}
