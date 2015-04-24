/* Copyright 2015 Outscale SAS
 *
 * This file is part of Butterfly.
 *
 * Butterfly is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as published
 * by the Free Software Foundation.
 *
 * Butterfly is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Butterfly.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <unistd.h>
#include <syslog.h>
#include <iostream>
#include <ctime>
#include <thread>
#include <fstream>
#include <string>
#include "api/server/app.h"
#include "api/server/server.h"

namespace app {

Log::Log() {
    // Set default log level
    set_log_level("error");

    // Openlog
    openlog("butterfly", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL0);
}

Log::~Log() {
    closelog();
}

bool Log::set_log_level(std::string level) {
    if (level == "none") {
        setlogmask(0);
    } else if (level == "error") {
        setlogmask(LOG_UPTO(LOG_ERR));
    } else if (level == "warning") {
        setlogmask(LOG_UPTO(LOG_WARNING));
    } else if (level == "info") {
        setlogmask(LOG_UPTO(LOG_INFO));
    } else if (level == "debug") {
        setlogmask(LOG_UPTO(LOG_DEBUG));
    } else {
        error("Log::set_log_level: non existing log level");
        return false;
    }
    return true;
}

std::string Log::build_details(const char *message, const char *file,
                               const char *func, int line) {
    std::string o;
    if (message != nullptr)
        o += std::string(message);
    if (file != nullptr)
        o += std::string(" ") + std::string(file);
    if (func != nullptr)
        o += std::string(" (") + std::string(func) + std::string(")");
    if (line != -1)
        o += std::string(":") + std::to_string(line);
    return o;
}

void Log::debug(const char *message, const char *file,
                const char *func, int line) {
    std::string o = build_details(message, file, func, line);
    syslog(LOG_DEBUG, "<debug> %s", o.c_str());
}

void Log::info(const char *message, const char *file,
               const char *func, int line) {
    std::string o = build_details(message, file, func, line);
    syslog(LOG_INFO, "<info> %s", o.c_str());
}

void Log::warning(const char *message, const char *file,
                  const char *func, int line) {
    std::string o = build_details(message, file, func, line);
    syslog(LOG_WARNING, "<warning> %s", o.c_str());
}

void Log::error(const char *message, const char *file,
                const char *func, int line) {
    std::string o = build_details(message, file, func, line);
    syslog(LOG_ERR, "<error> %s", o.c_str());
}

void Log::debug(const std::string &message, const char *file,
                const char *func, int line) {
    debug(message.c_str(), file, func, line);
}

void Log::info(const std::string &message, const char *file,
               const char *func, int line) {
    info(message.c_str(), file, func, line);
}

void Log::warning(const std::string &message, const char *file,
                  const char *func, int line) {
    warning(message.c_str(), file, func, line);
}

void Log::error(const std::string &message, const char *file,
                const char *func, int line) {
    error(message.c_str(), file, func, line);
}

// Global instances in app namespace
bool request_exit(false);
Log log;

}  // namespace app

int
main(int argc, char *argv[]) {
    try {
        app::log.info("butterfly starts");

        // Prepare & run libbutterfly
        // TODO(jerome.jutteau)

        // Prepare & run API server
        bool exit = false;
        APIServer server("tcp://0.0.0.0:9999", &exit);
        server.run_threaded();

        while (!exit)
            std::this_thread::sleep_for(std::chrono::seconds(1));
    } catch (std::exception & e) {
        LOG_ERROR_(e.what());
    }

    // Wait few seconds to give a chance to detached threads to exit cleanly
    std::this_thread::sleep_for(std::chrono::seconds(3));

    app::log.info("butterfly exit");
    return 0;
}
