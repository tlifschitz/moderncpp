#include <fstream>
#include <iostream>
#include <string>

/**
 * @file File.hpp
 * @brief RAII wrapper for file handling in C++
 */
class LogFile {
    std::string  filename;
    std::fstream fileStream;

  public:
    LogFile(const std::string& name) : filename(name) {
        fileStream.open(filename, std::ios::out | std::ios::app);
        if (!fileStream.is_open()) {
            throw std::runtime_error("Failed to open file: " + filename);
        }
        this->operator<<("LOG OPENED");
    }

    ~LogFile() {
        if (fileStream.is_open()) {
            this->operator<<("LOG CLOSED");
            fileStream.close();
        }
    }

    void operator<<(const std::string& data) {
        if (fileStream.is_open()) {
            std::time_t now = std::time(nullptr);
            std::tm     tm  = *std::localtime(&now);
            std::string d   = data;
            if (!d.empty() && d.back() == '\n') {
                d.pop_back();
                if (!d.empty() && d.back() == '\r')
                    d.pop_back();
            }
            fileStream << std::put_time(&tm, "[%Y-%m-%d %H:%M:%S] ") << d << "\n";
        }
    }
};