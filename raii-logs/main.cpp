#include <iostream>

#include "LogFile.hpp"

int main() {
    LogFile log("logs.txt");
    log << "Hello, RAII logfile :)";
    return 0;
}