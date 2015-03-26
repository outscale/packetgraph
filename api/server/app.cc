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
#include <glib.h>
#include <iostream>
#include <ctime>
#include <thread>
#include <fstream>
#include <string>
#include "api/server/app.h"
#include "api/server/server.h"
#include "api/protocol/revision.h"

namespace app {
Stats::Stats() {
    start_date = time(nullptr);
    request_counter = 0;
}

Config::Config() {
    api_endpoint = "tcp://0.0.0.0:9999";
    log_level = "error";
    show_revision = false;
}

bool Config::parse_cmd(int argc, char **argv) {
    auto gfree = [](gchar *p) { g_free(p); };
    std::unique_ptr<gchar, decltype(gfree)> config_path_cmd(nullptr, gfree);
    std::unique_ptr<gchar, decltype(gfree)> external_ip_cmd(nullptr, gfree);
    std::unique_ptr<gchar, decltype(gfree)> api_endpoint_cmd(nullptr, gfree);
    std::unique_ptr<gchar, decltype(gfree)> log_level_cmd(nullptr, gfree);
    std::unique_ptr<gchar, decltype(gfree)> pid_path_cmd(nullptr, gfree);
    std::unique_ptr<gchar, decltype(gfree)> socket_folder_cmd(nullptr, gfree);

    static GOptionEntry entries[] = {
        {"config", 'c', 0, G_OPTION_ARG_FILENAME, &config_path_cmd,
         "Path to configuration file", "FILE"},
        {"ip", 'i', 0, G_OPTION_ARG_STRING, &external_ip_cmd,
         "IP address to use", "IP_ADDRESS"},
        {"endpoint", 'e', 0, G_OPTION_ARG_STRING, &api_endpoint_cmd,
         "API endpoint to bind (default is 'tcp://0.0.0.0:9999')",
         "API_ENDPOINT"},
        {"log-level", 'l', 0, G_OPTION_ARG_STRING, &log_level_cmd,
         "Log level to use. LOG_LEVEL can be 'none', 'error' (default), " \
         "'warning', 'info' or 'debug'", "LOG_LEVEL"},
        {"revision", 'r', 0, G_OPTION_ARG_NONE, &show_revision,
         "Show the protocol revision number and exit", nullptr},
        {"pid", 'p', 0, G_OPTION_ARG_FILENAME, &pid_path_cmd,
         "Write PID of process in specified file", "FILE"},
        {"socket-dir", 's', 0, G_OPTION_ARG_FILENAME, &socket_folder_cmd,
         "Create network sockets in specified directory", "DIR"},
        { nullptr }
    };
    GOptionContext *context = g_option_context_new("");
    g_option_context_add_main_entries(context, entries, nullptr);

    GError *error = nullptr;
    if (!g_option_context_parse(context, &argc, &argv, &error)) {
        if (error != nullptr)
            std::cout << error->message << std::endl;
        return false;
    }

    // Get back gchar to config in std::string
    if (config_path_cmd != nullptr)
        config_path = std::string(&*config_path_cmd);
    if (external_ip_cmd != nullptr)
        external_ip = std::string(&*external_ip_cmd);
    if (api_endpoint_cmd != nullptr)
        api_endpoint = std::string(&*api_endpoint_cmd);
    if (log_level_cmd != nullptr)
        log_level = std::string(&*log_level_cmd);
    if (pid_path_cmd != nullptr)
        pid_path = std::string(&*pid_path_cmd);
    if (socket_folder_cmd != nullptr)
        socket_folder = std::string(&*socket_folder_cmd);

    return true;
}

bool Config::missing_mandatory() {
    bool ret = false;
    if (external_ip.length() == 0) {
        std::cerr << "IP to use is not set" << std::endl;
        app::log.error("IP to use is not set");
        ret = true;
    }
    if (socket_folder.length() == 0) {
        std::cerr << "socket folder is not set" << std::endl;
        app::log.error("socket folder is not set");
        ret = true;
    }

    if (ret)
        app::log.error("missing mandatory configuration items");
    return ret;
}

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
Config config;
Stats stats;
Model model;
Log log;

}  // namespace app

int
main(int argc, char *argv[]) {
    try {
        // Check parameters
        if (!app::config.parse_cmd(argc, argv))
            return 0;

        // Set log level from options
        app::log.set_log_level(app::config.log_level);

        // Ask for revision number ?
        if (app::config.show_revision) {
            std::cout << PROTOS_REVISION << std::endl;
            return 0;
        }

        // Ready to start ?
        if (app::config.missing_mandatory()) {
            std::cerr << "Some arguments are missing, please check " \
            "configuration or use --help" << std::endl;
            return 0;
        }

        app::log.info("butterfly starts");

        // Prepare & run libbutterfly
        // TODO(jerome.jutteau)

        // Prepare & run API server
        APIServer server(app::config.api_endpoint, &app::request_exit);
        server.run_threaded();

        while (!app::request_exit)
            std::this_thread::sleep_for(std::chrono::seconds(1));
    } catch (std::exception & e) {
        LOG_ERROR_(e.what());
    }

    // Wait few seconds to give a chance to detached threads to exit cleanly
    std::this_thread::sleep_for(std::chrono::seconds(3));

    app::log.info("butterfly exit");
    return 0;
}
