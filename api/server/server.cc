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
#include <thread>
#include "api/server/server.h"
#include "api/server/app.h"
#include "api/server/api.h"

APIServer::APIServer(std::string zmq_endpoint, bool *end_trigger) {
    endpoint_ = zmq_endpoint;
    end_ = end_trigger;
    prepare();
}

void
APIServer::run_threaded() {
    std::thread t(static_loop, this);
    t.detach();
}

void
APIServer::loop() {
    try {
        zmqpp::message request;
        zmqpp::message response;
        if (!socket_->receive(request, true))
            return;
        LOG_DEBUG_("ZMQ received a message");
        process(request, &response);
        LOG_DEBUG_("ZMQ send");
        if (!socket_->send(response, true)) {
            LOG_ERROR_("failed to send ZMQ message");
            return;
        }
    } catch (std::exception &e) {
        std::string m = "got exception: " + std::string(e.what());
        LOG_ERROR_(m);
    }
}

void
APIServer::run() {
    while (42) {
        loop();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (end_ != nullptr && *end_ == true)
            break;
    }
}

void
APIServer::prepare() {
    socket_ = std::make_shared < zmqpp::socket >(
        context_,
        zmqpp::socket_type::reply);
    socket_->bind(endpoint_);
}

void
APIServer::static_loop(APIServer *me) {
    if (me != NULL)
        me->run();
}

void
APIServer::process(const zmqpp::message &req, zmqpp::message *res) {
    // Extract message from ZMQ
    LOG_DEBUG_("unpack request from a ZMQ message");
    std::string request_string = req.get(0);

    // Process request
    std::string response_string;
    API::process_request(request_string, &response_string);

    // Pack message in ZMQ
    LOG_DEBUG_("pack response in a ZMQmessage");
    *res << response_string;
}

