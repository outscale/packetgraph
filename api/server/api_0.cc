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

#include <arpa/inet.h>
#include <algorithm>
#include "api/server/api.h"
#include "api/server/app.h"

void API_0::process(const MessageV0 &req, MessageV0 *res) {
    if (res == nullptr)
        return;
    res->set_allocated_response(new MessageV0_Response);

    // Set back message_id if set
    if (req.has_message_id())
        res->set_message_id(req.message_id());

    auto rs = res->mutable_response();
    if (!req.has_request()) {
        build_nok_res(rs, "MessageV0 appears to not have any request inside");
        return;
    }

    auto rq = req.request();
    if (rq.has_nic_add())
        nic_add(rq, rs);
    else if (rq.has_nic_update())
        nic_update(rq, rs);
    else if (rq.has_nic_list())
        nic_list(rq, rs);
    else if (rq.has_nic_details())
        nic_details(rq, rs);
    else if (rq.has_nic_export())
        nic_export(rq, rs);
    else if (rq.has_nic_stats())
        nic_stats(rq, rs);
    else if (rq.has_sg_add())
        sg_add(rq, rs);
    else if (rq.has_sg_del())
        sg_del(rq, rs);
    else if (rq.has_sg_list())
        sg_list(rq, rs);
    else if (rq.has_sg_rule_add())
        sg_rule_add(rq, rs);
    else if (rq.has_sg_rule_del())
        sg_rule_del(rq, rs);
    else if (rq.has_sg_rule_list())
        sg_rule_list(rq, rs);
    else if (rq.has_sg_member_add())
        sg_member_add(rq, rs);
    else if (rq.has_sg_member_del())
        sg_member_del(rq, rs);
    else if (rq.has_sg_member_list())
        sg_member_list(rq, rs);
    else if (rq.has_app_status())
        app_status(rq, rs);
    else if (rq.has_app_quit())
        app_quit(rq, rs);
    else if (rq.has_app_config())
        app_config(rq, rs);
    else
        build_nok_res(rs, "MessageV0 appears to not have any request");
}

void API_0::nic_add(const MessageV0_Request &req, MessageV0_Response *res) {
    if (res == nullptr)
        return;
    app::log.info("NIC added");
    auto n = req.nic_add();
    if (!validate_nic(n)) {
        build_nok_res(res, "Bad NIC format");
        return;
    }

    app::Nic nic;
    if (!convert(n, &nic)) {
        build_nok_res(res, "Internal error");
        return;
    }

    app::Error error;
    std::string path;
    if (!action_nic_add(nic, path, &error)) {
        build_nok_res(res, error);
        return;
    }

    res->set_allocated_nic_add(new MessageV0_NicAddRes);
    res->mutable_nic_add()->set_path(path);
    build_ok_res(res);
}

void API_0::nic_update(const MessageV0_Request &req,
    MessageV0_Response *res) {
    if (res == nullptr)
        return;
    app::log.info("NIC updated");
    auto n = req.nic_update();
    if (!validate_nic_update(n)) {
        build_nok_res(res, "Bad Nic update format");
        return;
    }

    API::NicUpdate nic_update;
    if (!convert(n, &nic_update)) {
        build_nok_res(res, "Internal error");
        return;
    }

    app::Error error;
    if (!action_nic_update(nic_update, &error)) {
        build_nok_res(res, error);
        return;
    }

    build_ok_res(res);
}

void API_0::nic_list(const MessageV0_Request &req, MessageV0_Response *res) {
    if (res == nullptr)
        return;
    app::log.info("NIC listing");
    build_ok_res(res);
    app::Model &m = app::model;
    for (auto it=m.nics.begin(); it != m.nics.end(); it++)
        res->add_nic_list(it->second.id);
}

void API_0::nic_details(const MessageV0_Request &req,
    MessageV0_Response *res) {
    if (res == nullptr)
        return;
    app::log.info("NIC details");
    app::Model &m = app::model;
    std::string id = req.nic_details();
    // Details of one NIC only
    if (id.length() > 0) {
        auto it = m.nics.find(id);
        if (it == m.nics.end()) {
            build_nok_res(res, "NIC not found");
            return;
        }

        auto n = res->add_nic_details();
        if (!convert(it->second, n)) {
            build_nok_res(res, "Internal error");
            return;
        }
        build_ok_res(res);
        return;
    }
    // Details of all NICs
    for (auto it=m.nics.begin(); it != m.nics.end(); it++) {
        auto n = res->add_nic_details();
        if (!convert(it->second, n)) {
            build_nok_res(res, "Internal error");
            return;
        }
    }
    build_ok_res(res);
}

void API_0::nic_export(const MessageV0_Request &req,
    MessageV0_Response *res) {
    if (res == nullptr)
        return;
    app::log.info("NIC export");
    std::string nic_id = req.nic_export();
    std::string nic_data;
    app::Error err;
    if (!action_nic_export(nic_id, &nic_data, &err)) {
        build_nok_res(res, err);
        return;
    }
    res->set_allocated_nic_export(new MessageV0_NicExportRes);
    auto nic_data_res = res->mutable_nic_export();

    nic_data_res->set_allocated_fw_data(new MessageV0_FwData);
    auto fw_data = nic_data_res->mutable_fw_data();

    fw_data->set_data(nic_data);
    build_ok_res(res);
}

void API_0::nic_stats(const MessageV0_Request &req,
    MessageV0_Response *res) {
    if (res == nullptr)
        return;
    app::log.info("NIC stats");
    std::string nic_id = req.nic_export();
    uint64_t in, out = 0;
    app::Error err;
    if (!action_nic_stats(nic_id, &in, &out, &err)) {
        build_nok_res(res, err);
        return;
    }

    res->set_allocated_nic_stats(new MessageV0_NicStats);
    auto nic_stats = res->mutable_nic_stats();

    nic_stats->set_in(in);
    nic_stats->set_out(out);
    build_ok_res(res);
}

void API_0::sg_add(const MessageV0_Request &req, MessageV0_Response *res) {
    if (res == nullptr)
        return;
    app::log.info("Security Group add");
    auto s = req.sg_add();

    if (!validate_sg(s)) {
        build_nok_res(res, "Bad Security Group format");
        return;
    }

    app::Sg sg;
    if (!convert(s, &sg)) {
        build_nok_res(res, "Internal error");
        return;
    }

    app::Error error;
    if (!action_sg_add(sg, &error))
        build_nok_res(res, error);
    else
        build_ok_res(res);
}

void API_0::sg_del(const MessageV0_Request &req, MessageV0_Response *res) {
    if (res == nullptr)
        return;
    app::log.info("Security group delete");
    std::string sg_id = req.sg_del();

    app::Error error;
    if (!action_sg_del(sg_id, &error))
        build_nok_res(res, error);
    else
        build_ok_res(res);
}

void API_0::sg_list(const MessageV0_Request &req, MessageV0_Response *res) {
    if (res == nullptr)
        return;
    app::log.info("Security Group listing");
    app::Model &m = app::model;
    for (auto it=m.security_groups.begin();
         it != m.security_groups.end(); it++) {
        auto id = res->add_sg_list();
        *id = it->second.id;
    }
    build_ok_res(res);
}

void API_0::sg_rule_add(const MessageV0_Request &req,
    MessageV0_Response *res) {
    if (res == nullptr)
        return;
    app::log.info("Security Group rule add");
    auto rule_req = req.sg_rule_add();
    auto r = rule_req.rule();
    if (!validate_sg_rule(r)) {
        build_nok_res(res, "Bad Security Group rule format");
        return;
    }

    std::string sg_id = rule_req.sg_id();
    app::Rule rule;
    if (!convert(r, &rule)) {
        build_nok_res(res, "Internal error");
        return;
    }

    app::Error error;
    if (!action_sg_rule_add(sg_id, rule, &error))
        build_nok_res(res, error);
    else
        build_ok_res(res);
}

void API_0::sg_rule_del(const MessageV0_Request &req, MessageV0_Response *res) {
    if (res == nullptr)
        return;
    app::log.info("Security Group rule deletion");
    auto rule_req = req.sg_rule_del();
    auto r = rule_req.rule();
    if (!validate_sg_rule(r)) {
        build_nok_res(res, "Bad Security Group rule format");
        return;
    }

    std::string sg_id = rule_req.sg_id();
    app::Rule rule;
    if (!convert(r, &rule)) {
        build_nok_res(res, "Internal error");
        return;
    }

    app::Error error;
    if (!action_sg_rule_del(sg_id, rule, &error))
        build_nok_res(res, error);
    else
        build_ok_res(res);
}

void API_0::sg_rule_list(const MessageV0_Request &req,
    MessageV0_Response *res) {
    if (res == nullptr)
        return;
    app::log.info("Security Group rule list");
    std::string sg_id = req.sg_rule_list();
    app::Model &m = app::model;

    if (m.security_groups.find(sg_id) == m.security_groups.end()) {
        build_nok_res(res, "Security Group not found");
        return;
    }

    app::Sg &sg = m.security_groups[sg_id];

    for (auto it=sg.rules.begin(); it != sg.rules.end(); it++) {
        auto r = res->add_sg_rule_list();
        if (!convert(it->second, r)) {
            build_nok_res(res, "Internal error");
            return;
        }
    }

    build_ok_res(res);
}

void API_0::sg_member_add(const MessageV0_Request &req,
    MessageV0_Response *res) {
    if (res == nullptr)
        return;
    app::log.info("Security Group member add");
    auto member_req = req.sg_member_add();
    auto i = member_req.member();
    if (!validate_ip(i)) {
        build_nok_res(res, "Bad IP format");
        return;
    }

    std::string sg_id = member_req.sg_id();
    app::Ip ip;
    if (!convert(i, &ip)) {
        build_nok_res(res, "Internal error");
        return;
    }

    app::Error error;
    if (!action_sg_member_add(sg_id, ip, &error))
        build_nok_res(res, error);
    else
        build_ok_res(res);
}

void API_0::sg_member_del(const MessageV0_Request &req,
    MessageV0_Response *res) {
    if (res == nullptr)
        return;
    app::log.info("Security Group member deletion");
    auto member_req = req.sg_member_del();
    auto i = member_req.member();
    if (!validate_ip(i)) {
        build_nok_res(res, "Bad IP format");
        return;
    }

    std::string sg_id = member_req.sg_id();
    app::Ip ip;
    if (!convert(i, &ip)) {
        build_nok_res(res, "Internal error");
        return;
    }

    app::Error error;
    if (!action_sg_member_del(sg_id, ip, &error))
        build_nok_res(res, error);
    else
        build_ok_res(res);
}

void API_0::sg_member_list(const MessageV0_Request &req,
    MessageV0_Response *res) {
    if (res == nullptr)
        return;
    app::log.info("Security Group member list");
    std::string sg_id = req.sg_member_list();
    app::Model &m = app::model;

    if (m.security_groups.find(sg_id) == m.security_groups.end()) {
        build_nok_res(res, "Security Group not found");
        return;
    }

    app::Sg &sg = m.security_groups[sg_id];

    for (auto it=sg.members.begin(); it != sg.members.end(); it++) {
        auto m = res->add_sg_member_list();
        if (!convert(*it, m)) {
            build_nok_res(res, "Internal error");
            return;
        }
    }
    build_ok_res(res);
}

void API_0::app_status(const MessageV0_Request &req,
    MessageV0_Response *res) {
    if (res == nullptr)
        return;
    app::log.info("Application status");
    res->set_allocated_app_status(new MessageV0_AppStatusRes);
    auto a = res->mutable_app_status();
    a->set_start_date(app::stats.start_date);
    a->set_current_date(time(NULL));
    a->set_request_counter(app::stats.request_counter);
    a->set_graph_dot(action_graph_dot());

    build_ok_res(res);
}

void API_0::app_quit(const MessageV0_Request &req, MessageV0_Response *res) {
    if (res == nullptr)
        return;
    app::log.info("Application quit");
    action_app_quit();
    build_ok_res(res);
}

void API_0::app_config(const MessageV0_Request &req, MessageV0_Response *res) {
    if (res == nullptr)
        return;
    app::log.info("Application configuration");
    auto config = req.app_config();
    std::string err;
    if (config.has_log_level()) {
        if (!app::log.set_log_level(config.log_level()))
            err = err + "log level: failed ";
    }

    if (err.length() == 0) {
        build_ok_res(res);
    } else {
        err = "Application configuration failed: " + err;
        build_nok_res(res, err);
    }
}

void API_0::build_ok_res(MessageV0_Response *res) {
    if (res == nullptr)
        return;
    auto s = res->mutable_status();
    s->set_status(true);
}

void API_0::build_nok_res(MessageV0_Response *res) {
    if (res == nullptr)
        return;
    auto s = res->mutable_status();
    s->set_status(false);
}

void API_0::build_nok_res(MessageV0_Response *res, std::string description) {
    if (res == nullptr)
        return;
    LOG_DEBUG_(description);
    build_nok_res(res);
    auto s = res->mutable_status();
    s->set_allocated_error(new MessageV0_Error);
    auto e = s->mutable_error();
    e->set_description(description);
}

void API_0::build_nok_res(MessageV0_Response *res, const char *description) {
    std::string d;
    if (description != nullptr)
        d = description;
    build_nok_res(res, d);
}

void API_0::build_nok_res(MessageV0_Response *res, const app::Error &error) {
    if (res == nullptr)
        return;
    auto s = res->mutable_status();
    s->set_allocated_error(new MessageV0_Error);
    auto e = s->mutable_error();

    if (!convert(error, e)) {
        s->release_error();
        build_nok_res(res, "Internal Error");
    }
    build_nok_res(res);
}

bool API_0::validate_nic(const MessageV0_Nic &nic) {
    // Check MAC
    if (!validate_mac(nic.mac()))
        return false;

    // Check VNI
    uint32_t vni = nic.vni();
    if ((vni & (((uint32_t)-1) >> 8)) != vni)
        return false;

    // Check IP list
    for (int a = 0; a < nic.ip_size(); a++) {
        auto ip = nic.ip(a);
        if (!validate_ip(ip))
            return false;
    }
    return true;
}

bool API_0::validate_nic_update(const MessageV0_NicUpdateReq &nic_update) {
    // Check IP list
    for (int a = 0; a < nic_update.ip_size(); a++) {
        auto ip = nic_update.ip(a);
        if (!validate_ip(ip))
            return false;
    }
    return true;
}

bool API_0::validate_sg(const MessageV0_Sg &sg) {
    // Check Members
    for (int a = 0; a < sg.member_size(); a++) {
        auto ip = sg.member(a);
        if (!validate_ip(ip))
            return false;
    }

    // Check Rules
    for (int a = 0; a < sg.rule_size(); a++) {
        auto rule = sg.rule(a);
        if (!validate_sg_rule(rule))
            return false;
    }
    return true;
}

bool API_0::validate_sg_rule(const MessageV0_Rule &rule) {
    // Check protocol
    int32_t protocol = rule.protocol();
    if (protocol < -1 || protocol > 255) {
        LOG_DEBUG_("protol number out of range");
        return false;
    }

    // Check that we have port_start and port_end if using TCP or UDP
    if (protocol == 6 || protocol == 17) {
        if (!rule.has_port_start() || !rule.has_port_end()) {
            LOG_DEBUG_("missing port informations");
            return false;
        }

        // Check port_start and port_end
        uint32_t port_start = rule.port_start();
        uint32_t port_end = rule.port_end();
        if (port_start > 65535 || port_end > 65535) {
            LOG_DEBUG_("bad port range");
            return false;
        }
    }

    // CIDR or security group
    if (rule.has_security_group() && rule.has_cidr()) {
        LOG_DEBUG_("cannot have CIDR and SG specified");
        return false;
    }

    // Validate CIDR
    if (rule.has_cidr()) {
        auto cidr = rule.cidr();
        if (!validate_cidr(cidr))
            return false;
    }

    return true;
}

bool API_0::validate_ip(const std::string &ip) {
    struct in6_addr a6;
    struct in_addr a4;
    const char *s = ip.c_str();
    if (inet_pton(AF_INET6, s, &a6) != 1 &&
        inet_pton(AF_INET, s, &a4) != 1)
        return false;
    return true;
}

bool API_0::validate_mac(const std::string &mac) {
    if (mac.length() != 17)
        return false;
    const char *cstr = mac.c_str();
    unsigned int addr[6];
    if (sscanf(cstr, "%2x:%2x:%2x:%2x:%2x:%2x",
               &addr[0], &addr[1], &addr[2],
               &addr[3], &addr[4], &addr[5]) != 6)
        return false;
    for (int i = 0; i < 6; i++)
        if (addr[i] < 0 || addr[i] > 255)
            return false;
    return true;
}

bool API_0::validate_cidr(const MessageV0_Cidr &cidr) {
    const char *ip = cidr.address().c_str();
    struct in6_addr a6;
    struct in_addr a4;
    if (inet_pton(AF_INET6, ip, &a6) == 1) {
        if (cidr.mask_size() > 128)
            return false;
    } else if (inet_pton(AF_INET, ip, &a4) == 1) {
        if (cidr.mask_size() > 32)
            return false;
    } else {
        return false;
    }
    return true;
}

bool API_0::convert(const app::Nic &nic_model, MessageV0_Nic *nic_message) {
    if (nic_message == nullptr)
        return false;
    // Id
    nic_message->set_id(nic_model.id);
    // MAC
    auto mac = nic_message->mutable_mac();
    if (!convert(nic_model.mac, mac))
        return false;
    // VNI
    nic_message->set_vni(nic_model.vni);
    // IP list of NIC
    for (auto it = nic_model.ip_list.begin();
         it != nic_model.ip_list.end(); it++) {
        auto ip = nic_message->add_ip();
        if (!convert(*it, ip))
            return false;
    }
    // List of security groups
    for (auto it = nic_model.security_groups.begin();
         it != nic_model.security_groups.end(); it++) {
        auto sg = nic_message->add_security_group();
        *sg = *it;
    }
    // Antispoof IP
    nic_message->set_ip_anti_spoof(nic_model.ip_anti_spoof);
    // Sniff target
    if (nic_model.sniff_target_nic_id.length() > 0)
        nic_message->set_sniff_target_nic_id(nic_model.sniff_target_nic_id);
    return true;
}

bool API_0::convert(const MessageV0_Nic &nic_message, app::Nic *nic_model) {
    if (nic_model == nullptr)
        return false;
    // Id
    nic_model->id = nic_message.id();
    // MAC
    if (!convert(nic_message.mac(), &nic_model->mac))
        return false;
    // VNI
    nic_model->vni = nic_message.vni() & (((uint32_t)-1) >> 8);
    // IP list of NIC
    for (int a = 0; a < nic_message.ip_size(); a++) {
        app::Ip ip;
        if (!convert(nic_message.ip(a), &ip))
            return false;
        // Don't add IP if we already have it
        auto res = std::find(nic_model->ip_list.begin(),
                             nic_model->ip_list.end(), ip);
        if (res != nic_model->ip_list.end())
            continue;
        nic_model->ip_list.push_back(ip);
    }
    // List of security groups
    for (int a = 0; a < nic_message.security_group_size(); a++) {
        // Don't add security group if we already have it
        std::string sg_id = nic_message.security_group(a);
        auto res = std::find(nic_model->security_groups.begin(),
                             nic_model->security_groups.end(), sg_id);
        if (res != nic_model->security_groups.end())
            continue;
        nic_model->security_groups.push_back(sg_id);
    }
    // Antispoof IP
    nic_model->ip_anti_spoof = false;
    if (nic_message.has_ip_anti_spoof())
        nic_model->ip_anti_spoof = nic_message.ip_anti_spoof();
    // Sniff target
    if (nic_message.has_sniff_target_nic_id())
        nic_model->sniff_target_nic_id = nic_message.sniff_target_nic_id();
    return true;
}

bool API_0::convert(const MessageV0_NicUpdateReq &nic_update_message,
                    API::NicUpdate *nic_update_model) {
    if (nic_update_model == nullptr)
        return false;
    // Id
    nic_update_model->id = nic_update_message.id();
    // Antispoof IP
    if (nic_update_message.has_ip_anti_spoof()) {
        nic_update_model->ip_anti_spoof = nic_update_message.ip_anti_spoof();
        nic_update_model->has_ip_anti_spoof = true;
    } else {
        nic_update_model->has_ip_anti_spoof = false;
    }
    // IP list of NIC
    for (int a = 0; a < nic_update_message.ip_size(); a++) {
        app::Ip ip;
        if (!convert(nic_update_message.ip(a), &ip))
            return false;

        // Don't add IP if we already have it
        auto res = std::find(nic_update_model->ip.begin(),
                             nic_update_model->ip.end(), ip);
        if (res != nic_update_model->ip.end())
            continue;
        nic_update_model->ip.push_back(ip);
    }
    // List Security groups
    for (int a = 0; a < nic_update_message.security_group_size(); a++) {
        // Don't add security group if we already have it
        std::string sg_id = nic_update_message.security_group(a);
        auto res = std::find(nic_update_model->security_groups.begin(),
                             nic_update_model->security_groups.end(), sg_id);
        if (res != nic_update_model->security_groups.end())
            continue;
        nic_update_model->security_groups.push_back(sg_id);
    }
    return true;
}

bool API_0::convert(const app::Sg &sg_model, MessageV0_Sg *sg_message) {
    if (sg_message == nullptr)
        return false;
    // Id
    sg_message->set_id(sg_model.id);
    // List of members
    for (auto it = sg_model.members.begin();
         it != sg_model.members.end(); it++) {
        auto m = sg_message->add_member();
        if (!convert(*it, m))
            return false;
    }
    // List of rules
    for (auto it = sg_model.rules.begin();
         it != sg_model.rules.end(); it++) {
        auto r = sg_message->add_rule();
        if (!convert(it->second, r))
            return false;
    }
    return true;
}

bool API_0::convert(const MessageV0_Sg &sg_message, app::Sg *sg_model) {
    if (sg_model == nullptr)
        return false;
    // Id
    sg_model->id = sg_message.id();
    // List of members
    for (int a = 0; a < sg_message.member_size(); a++) {
        app::Ip ip;
        if (!convert(sg_message.member(a), &ip))
            return false;

        // Don't add IP if we already have it
        auto res = std::find(sg_model->members.begin(),
                             sg_model->members.end(), ip);
        if (res != sg_model->members.end())
            continue;

        sg_model->members.push_back(ip);
    }
    // List of rules
    for (int a = 0; a < sg_message.rule_size(); a++) {
        app::Rule rule;
        if (!convert(sg_message.rule(a), &rule))
            return false;
        std::hash<app::Rule> hf;
        std::size_t h = hf(rule);
        // Don't add rule if already exist
        if (sg_model->rules.find(h) != sg_model->rules.end())
            continue;
        std::pair<std::size_t, app::Rule> p(h, rule);
        sg_model->rules.insert(p);
    }
    return true;
}

bool API_0::convert(const app::Rule &rule_model,
    MessageV0_Rule *rule_message) {
    if (rule_message == nullptr)
        return false;
    // Direction
    if (rule_model.direction == app::Rule::INBOUND)
        rule_message->set_direction(MessageV0_Rule_Direction_INBOUND);
    else if (rule_model.direction == app::Rule::OUTBOUND)
        rule_message->set_direction(MessageV0_Rule_Direction_OUTBOUND);
    else
        return false;
    // IP protocol
    rule_message->set_protocol(rule_model.protocol);
    // fill up ports for TCP and UDP
    if (rule_model.protocol == 6 || rule_model.protocol == 17) {
        if (rule_model.port_start == - 1 || rule_model.port_end == -1)
            return false;
        rule_message->set_port_start(rule_model.port_start);
        rule_message->set_port_end(rule_model.port_end);
    }
    // Convert CIDR _OR_ security_group options
    if (rule_model.security_group.length() > 0) {
        rule_message->set_security_group(rule_model.security_group);
    } else {
        rule_message->set_allocated_cidr(new MessageV0_Cidr);
        auto cidr = rule_message->mutable_cidr();
        if (!convert(rule_model.cidr, cidr))
            return false;
    }
    return true;
}

bool API_0::convert(const MessageV0_Rule &rule_message, app::Rule *rule_model) {
    if (rule_model == nullptr)
        return false;
    // Direction
    auto d = rule_message.direction();
    if (d == MessageV0_Rule_Direction_INBOUND)
        rule_model->direction = app::Rule::INBOUND;
    else if (d == MessageV0_Rule_Direction_OUTBOUND)
        rule_model->direction = app::Rule::OUTBOUND;
    // IP protocol
    rule_model->protocol = rule_message.protocol();
    // Start and End port if set
    if (rule_message.has_port_start())
        rule_model->port_start = rule_message.port_start();
    if (rule_message.has_port_end())
        rule_model->port_end = rule_message.port_end();
    // Convert CIDR _OR_ security_group options
    if (rule_message.has_security_group()) {
        rule_model->security_group = rule_message.security_group();
    } else if (rule_message.has_cidr()) {
        if (!convert(rule_message.cidr(), &rule_model->cidr))
            return false;
    } else {
        return false;
    }
    return true;
}

bool API_0::convert(const app::Ip &ip_model, std::string *ip_message) {
    if (ip_message == nullptr)
        return false;
    *ip_message = ip_model.str();
    return true;
}

bool API_0::convert(const std::string &ip_message, app::Ip *ip_model) {
    if (ip_model == nullptr)
        return false;
    *ip_model = ip_message;
    return true;
}

bool API_0::convert(const app::Mac &mac_model, std::string *mac_message) {
    if (mac_message == nullptr)
        return false;
    *mac_message = mac_model.str();
    return true;
}

bool API_0::convert(const std::string &mac_message, app::Mac *mac_model) {
    if (mac_model == nullptr)
        return false;
    *mac_model = mac_message;
    return true;
}

bool API_0::convert(const app::Cidr &cidr_model, MessageV0_Cidr *cidr_message) {
    if (cidr_message == nullptr)
        return false;
    // Address
    auto ip = cidr_message->mutable_address();
    if (!convert(cidr_model.address, ip))
        return false;
    // Address mask
    cidr_message->set_mask_size(cidr_model.mask_size);
    return true;
}

bool API_0::convert(const MessageV0_Cidr &cidr_message, app::Cidr *cidr_model) {
    if (cidr_model == nullptr)
        return false;
    // Address
    if (!convert(cidr_message.address(), &cidr_model->address))
        return false;
    // Address mask
    cidr_model->mask_size = cidr_message.mask_size();
    return true;
}

bool API_0::convert(const app::Error &error_model,
    MessageV0_Error *error_message) {
    if (error_message == nullptr)
        return false;
    // description
    if (error_model.description.length() > 0)
        error_message->set_description(error_model.description);
    // err_no
    if (error_model.has_err_no)
        error_message->set_err_no(error_model.err_no);
    // file
    if (error_model.file.length() > 0)
        error_message->set_file(error_model.file);
    // line
    if (error_model.has_line)
        error_message->set_line(error_model.line);
    // curs_pos
    if (error_model.has_curs_pos)
        error_message->set_curs_pos(error_model.curs_pos);
    // function
    if (error_model.function.length() > 0)
        error_message->set_function(error_model.function);
    return true;
}

