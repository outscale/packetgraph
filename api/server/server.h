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

#ifndef API_SERVER_SERVER_H_
#define API_SERVER_SERVER_H_

#include <zmqpp/zmqpp.hpp>
#include <string>
#include <memory>

class APIServer {
 public:
    APIServer(std::string zmq_endpoint, bool *end_trigger);
    void run();
    void run_threaded();

 private:
    inline void prepare();
    inline void loop();
    inline void process(const zmqpp::message &req, zmqpp::message *res);
    static void static_loop(APIServer *me);
    std::string endpoint_;
    zmqpp::context context_;
    std::shared_ptr <zmqpp::socket> socket_;
    bool *end_;
};

#endif  // API_SERVER_SERVER_H_
