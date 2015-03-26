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

#include <google/protobuf/text_format.h>
#include <iostream>
#include <functional>
#include <string>
#include <utility>
#include <algorithm>
#include "api/server/api.h"
#include "api/server/app.h"
#include "api/server/model.h"
#include "api/protocol/message.pb.h"
#include "api/protocol/revision.h"

void API::process_request(const std::string &request, std::string *response) {
    if (response == nullptr)
        return;
    // Increment request counter
    app::stats.request_counter++;

    // Request printer
    std::string human_message;
    google::protobuf::TextFormat::Printer printer;

    // Decode request
    proto::Messages reqs;
    if (!reqs.ParseFromString(request) || !reqs.IsInitialized()) {
        LOG_ERROR_("error while decoding request");
        proto::Message r;
        r.set_revision(PROTOS_REVISION);
        r.set_allocated_error(new proto::Error);
        r.mutable_error()->set_code(proto::Error_Code_PARSE_ERROR);
        r.SerializeToString(response);
        printer.PrintToString(r, &human_message);
        app::log.debug(human_message);
        return;
    }

    // Log request
    printer.PrintToString(reqs, &human_message);
    app::log.debug(human_message);

    // Check empty request
    if (reqs.messages_size() == 0) {
        LOG_WARNING_("received empty message");
        proto::Message rep;
        rep.set_revision(PROTOS_REVISION);
        rep.set_allocated_error(new proto::Error);
        rep.mutable_error()->set_code(proto::Error_Code_EMPTY_REQUEST);
        rep.SerializeToString(response);
        printer.PrintToString(rep, &human_message);
        app::log.debug(human_message);
        return;
    }

    // Dispatch all messages
    proto::Messages reps;
    for (int i = 0; i < reqs.messages_size(); i++) {
        auto rep = reps.add_messages();
        auto req = reqs.messages(i);
        dispatch(req, rep);
    }
    reps.SerializeToString(response);

    // Log response
    printer.PrintToString(reps, &human_message);
    app::log.debug(human_message);
}

void API::dispatch(const proto::Message &req, proto::Message *rep) {
    if (rep == nullptr)
        return;
    // Put the current revision number of the protocol in response
    rep->set_revision(PROTOS_REVISION);

    if (req.has_message_0()) {
        LOG_DEBUG_("dispatch to MessageV0");
        rep->set_allocated_message_0(new MessageV0);
        auto req_0 = req.message_0();
        auto rep_0 = rep->mutable_message_0();
        API_0::process(req_0, rep_0);
    } else {
        LOG_ERROR_("message version not supported");
        rep->set_allocated_error(new proto::Error);
        rep->mutable_error()->set_code(proto::Error_Code_VERSION_NOT_MANAGED);
    }
}

bool API::action_nic_add(const app::Nic &nic, std::string path,
    app::Error *error) {
    // Do we already have this NIC ?
    if (app::model.nics.find(nic.id) != app::model.nics.end()) {
        std::string m = "NIC already exists with id " + nic.id;
        app::log.warning(m);
        // Disable NIC in packetgraph
        // TODO(jerome.jutteau)
        // Remove NIC from model
        app::model.nics.erase(nic.id);
        // Retry !
        return API::action_nic_add(nic, path, error);
    }

    // TODO(jerome.jutteau)
    path = "...";

    // Add NIC in model
    std::pair<std::string, app::Nic> p(nic.id, nic);
    app::model.nics.insert(p);
    return true;
}

bool API::action_nic_update(const API::NicUpdate &update,
    app::Error *error) {
    // Do we have this NIC ?
    auto itn = app::model.nics.find(update.id);
    if (itn == app::model.nics.end()) {
        std::string m = "NIC does not exist with id " + update.id;
        app::log.error(m);
        if (error != nullptr)
            error->description = m;
        return false;
    }

    // TODO(jerome.jutteau)

    // Update NIC informations in model
    app::Nic &n = itn->second;
    n.ip_anti_spoof = update.ip_anti_spoof;
    n.ip_list = update.ip;
    n.security_groups = update.security_groups;
    return true;
}

bool API::action_nic_del(std::string id, app::Error *error) {
    // Do we have this NIC ?
    if (app::model.nics.find(id) == app::model.nics.end()) {
        std::string m = "NIC does not exist with this id " + id;
        app::log.warning(m);
        return true;
    }

    // TODO(jerome.jutteau)

    // Remove NIC from model
    app::model.nics.erase(id);

    return true;
}


bool API::action_nic_export(std::string id, std::string *data,
    app::Error *error) {
    if (data == nullptr)
        return false;
    // Do we have this NIC ?
    if (app::model.nics.find(id) == app::model.nics.end()) {
        std::string m = "NIC does not exist with id " + id;
        app::log.error(m);
        if (error != nullptr)
            error->description = m;
        return false;
    }

    // TODO(jerome.jutteau)

    return true;
}

bool API::action_nic_stats(std::string id, uint64_t *in, uint64_t *out,
    app::Error *error) {
    if (in == nullptr || out == nullptr)
        return false;
    // TODO(jerome.jutteau)
    *in = 42;
    *out = 42;
    return true;
}

bool API::action_sg_add(const app::Sg &sg, app::Error *error) {
    // Do we already have this security group ?
    if (app::model.security_groups.find(sg.id) !=
        app::model.security_groups.end()) {
        std::string m = "Security group already exists with this id " + sg.id;
        app::log.warning(m);
        // TODO(jerome.jutteau) replace
    } else {
        // TODO(jerome.jutteau) add
        std::pair<std::string, app::Sg> p(sg.id, sg);
        app::model.security_groups.insert(p);
    }
    return true;
}

bool API::action_sg_del(std::string id, app::Error *error) {
    // Do we have this security group ?
    if (app::model.security_groups.find(id) ==
        app::model.security_groups.end()) {
        std::string m = "Security group does not exist with this id " + id;
        app::log.warning(m);
        return true;
    }

    // TODO(jerome.jutteau)

    // Remove security group from model
    app::model.security_groups.erase(id);

    return true;
}


bool API::action_sg_rule_add(std::string sg_id, const app::Rule &rule,
    app::Error *error) {
    // Do we have this security group ?
    auto m = app::model.security_groups.find(sg_id);
    if (m == app::model.security_groups.end()) {
        std::string m = "Security group does not exist with this id " + sg_id;
        app::log.warning(m);
        // Create missing security group
        app::Sg nsg;
        nsg.id = sg_id;
        std::pair<std::string, app::Sg> p(sg_id, nsg);
        app::model.security_groups.insert(p);
        // Try again
        return API::action_sg_rule_add(sg_id, rule, error);
    }

    app::Sg &sg = m->second;

    // Compute rule's hash
    std::hash<app::Rule> hf;
    std::size_t h = hf(rule);

    // Does rule already exist in security group ?
    if (sg.rules.find(h) != sg.rules.end()) {
        std::string m = "Rule already exist in security group " + sg_id;
        app::log.warning(m);
        return true;
    }

    // TODO(jerome.jutteau)

    // Add rule to security group
    std::pair<std::size_t, app::Rule> p(h, rule);
    sg.rules.insert(p);
    return true;
}

bool API::action_sg_rule_del(std::string sg_id, const app::Rule &rule,
    app::Error *error) {
    // Do we have this security group ?
    auto m = app::model.security_groups.find(sg_id);
    if (m == app::model.security_groups.end()) {
        std::string m = "Can't delete rule from a non-existing security " \
            "group " + sg_id;
        app::log.warning(m);
        return true;
    }

    app::Sg &sg = m->second;

    // Compute rule's hash
    std::hash<app::Rule> hf;
    std::size_t h = hf(rule);

    // Does rule exist in security group ?
    if (sg.rules.find(h) == sg.rules.end()) {
        std::string m = "Can't delete non-existing rule from security " \
           "group " + sg_id;
        app::log.warning(m);
        return true;
    }

    // TODO(jerome.jutteau)

    // Remove rule from security group
    sg.rules.erase(h);
    return true;
}


bool API::action_sg_member_add(std::string sg_id, const app::Ip &ip,
    app::Error *error) {
    // Do we have this security group ?
    auto m = app::model.security_groups.find(sg_id);
    if (m == app::model.security_groups.end()) {
        std::string ms = "Security group does not exist with this id " + sg_id;
        app::log.warning(ms);
        // Create missing security group
        app::Sg nsg;
        nsg.id = sg_id;
        std::pair<std::string, app::Sg> p(sg_id, nsg);
        app::model.security_groups.insert(p);
        m = app::model.security_groups.find(sg_id);
        if (m == app::model.security_groups.end()) {
            ms = "Really cannot create security group with id" + sg_id;
            LOG_ERROR_(ms);
            return false;
        }
    }

    app::Sg &sg = m->second;

    // Does member already exist in security group ?
    auto res = std::find(sg.members.begin(), sg.members.end(), ip);
    if (res != sg.members.end()) {
        std::string m = "member " + ip.str() + " already exist in security " \
            "group " + sg_id;
        app::log.warning(m);
        return true;
    }

    // TODO(jerome.jutteau)

    // Add member to security group
    sg.members.push_back(ip);

    return true;
}


bool API::action_sg_member_del(std::string sg_id, const app::Ip &ip,
    app::Error *error) {
    // Do we have this security group ?
    auto m = app::model.security_groups.find(sg_id);
    if (m == app::model.security_groups.end()) {
        std::string m = "Can't delete member from a non-existing security " \
            "group " + sg_id;
        app::log.warning(m);
        return true;
    }

    app::Sg &sg = m->second;

     // Does member exist in security group ?
    auto res = std::find(sg.members.begin(), sg.members.end(), ip);
    if (res == sg.members.end()) {
        std::string m = "Can't delete non-existing member from security " \
            "group " + sg_id;
        app::log.warning(m);
        return true;
    }

    // TODO(jerome.jutteau)

    // Delete member from security group
    sg.members.erase(res);
    return true;
}

std::string API::action_graph_dot() {
    // TODO(jerome.jutteau)
    return "graph butterfly { \n" \
        "nic_0 [label=\"id-1\" shape=box]; \n" \
        "nic_1 [label=\"id-2\" shape=box]; \n" \
        "fw_id_0 [label=\"id-1\" shape=box color=red]; \n" \
        "fw_id_1 [label=\"id-2\" shape=box color=red]; \n" \
        "switch_l2 [label\"Switch\"]; \n" \
        "vxlan [label\"VXLAN\"]; \n" \
        "dpdk [label\"DPDK\"]; \n" \
        "nic_0 -- fw_id_0; \n" \
        "nic_1 -- fw_id_1; \n" \
        "fw_id_0 -- switch_l2; \n" \
        "fw_id_1 -- switch_l2; \n" \
        "switch_l2 -- vxlan; \n" \
        "vxlan -- dpdk; \n" \
        "}";
}

void API::action_app_quit() {
    app::request_exit = true;
}

