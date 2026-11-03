// Menu-driven pricing console.
//
// Usage: pricing_engine [--data DIR] [--date YYYY-MM-DD]
//   --data  folder with catalog.json, customers.json, rules.json, rules.txt
//           (default: the project's data/ folder)
//   --date  order date to price for (default: today)

#include <ctime>
#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include "application/ConflictPolicies.hpp"
#include "domain/Date.hpp"
#include "domain/TimeOfDay.hpp"
#include "infrastructure/FileLogSink.hpp"
#include "infrastructure/Logger.hpp"
#include "presentation/CliApp.hpp"

#ifndef DATA_DIR
#define DATA_DIR "data"
#endif

namespace {

domain::Date today() {
    const std::time_t now = std::time(nullptr);
    const std::tm* local = std::localtime(&now);
    return domain::Date(local->tm_year + 1900, local->tm_mon + 1, local->tm_mday);
}

domain::TimeOfDay now() {
    const std::time_t t = std::time(nullptr);
    const std::tm* local = std::localtime(&t);
    return domain::TimeOfDay(local->tm_hour, local->tm_min);
}

void usage() {
    std::cerr << "usage: pricing_engine [--data DIR] [--date YYYY-MM-DD] [--time HH:MM]"
                 " [--log FILE]\n";
}

} // namespace

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8); // so "₹" prints correctly
#endif
    try {
        presentation::AppConfig config{DATA_DIR, today(), now()};
        infrastructure::Logger logger; // silent unless --log is given
        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if ((arg == "--data" || arg == "--date" || arg == "--time" || arg == "--log") &&
                i + 1 < argc) {
                const std::string value = argv[++i];
                if (arg == "--data") config.dataDir = value;
                else if (arg == "--date") config.orderDate = domain::Date::fromString(value);
                else if (arg == "--time") config.orderTime = domain::TimeOfDay::fromString(value);
                else logger.addSink(std::make_unique<infrastructure::FileLogSink>(value));
            } else {
                usage();
                return 2;
            }
        }
        presentation::CliApp app(std::cin, std::cout, config,
                                 application::standardConflictPolicies(), std::move(logger));
        return app.run();
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << '\n';
        return 1;
    }
}
