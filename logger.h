#include <iostream>
#include <fstream>
#include <string>

std::mutex writemtx;
class Logger {
public:
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    void writeLog(const std::string& message) {
        // to stdout
        // std::cout << "[DEBUG] " << message << std::endl;

        // to logfile
        std::unique_lock<std::mutex> lock(writemtx);
        if (logFile.is_open()) {
            logFile << "[DEBUG] " << message << std::endl;
        }
    }

private:
    std::ofstream logFile;

    // open logfile
    Logger() {
        logFile.open("log.txt");
        if (!logFile) {
            std::cerr << "[ERROR] Failed to open log file!" << std::endl;
        }
    }

    // close logfile
    ~Logger() {
        if (logFile.is_open()) {
            logFile.close();
        }
    }

    // not to modify or copy
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
};