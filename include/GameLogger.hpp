#ifndef GAME_LOGGER_HPP
#define GAME_LOGGER_HPP

#include <string>

class GameLogger {
public:
    static void Init();
    static void Shutdown();
    static const std::string& GetLogFilePath();

private:
    static std::string s_LogFilePath;
};

#endif
