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

#ifndef API_SERVER_APP_H_
#define API_SERVER_APP_H_

#include <string>
#include "api/server/model.h"

// Usefull macros
#define LOG_PRINT_(str, method) \
    method(str, __FILE__, __PRETTY_FUNCTION__, __LINE__)
#define LOG_DEBUG_(str) LOG_PRINT_(str, app::log.debug)
#define LOG_INFO_(str) LOG_PRINT_(str, app::log.info)
#define LOG_WARNING_(str) LOG_PRINT_(str, app::log.warning)
#define LOG_ERROR_(str) LOG_PRINT_(str, app::log.error)

namespace app {
struct Config {
    Config();
    bool parse_cmd(int argc, char **argv);
    bool missing_mandatory();
    std::string external_ip;
    std::string config_path;
    std::string pid_path;
    std::string api_endpoint;
    std::string log_level;
    std::string socket_folder;
    bool show_revision;
};

struct Stats {
    Stats();
    uint64_t start_date;
    uint64_t request_counter;
};

class Log {
 public:
    Log();
    ~Log();
    bool set_log_level(std::string level);
    void debug(const char *message, const char *file = NULL,
               const char *func = NULL, int line = -1);
    void debug(const std::string &message, const char *file = NULL,
               const char *func = NULL, int line = -1);
    void info(const char *message, const char *file = NULL,
              const char *func = NULL, int line = -1);
    void info(const std::string &message, const char *file = NULL,
              const char *func = NULL, int line = -1);
    void warning(const char *message, const char *file = NULL,
                 const char *func = NULL, int line = -1);
    void warning(const std::string &message, const char *file = NULL,
                 const char *func = NULL, int line = -1);
    void error(const char *message, const char *file = NULL,
               const char *func = NULL, int line = -1);
    void error(const std::string &message, const char *file = NULL,
               const char *func = NULL, int line = -1);
 private:
    std::string build_details(const char *message, const char *file,
                              const char *func, int line);
};

// Manage signals
void SignalRegister();
void SignalHandler(int signum);

// Manage configuration file
bool LoadConfigFile(std::string config_path);

// Manage PID
void WritePid(std::string pid_path);

// Some global app:: variables
extern bool request_exit;
extern Config config;
extern Stats stats;
extern Model model;
extern Log log;
}  // namespace app

#endif  // API_SERVER_APP_H_
