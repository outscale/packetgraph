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

#ifndef API_SERVER_API_H_
#define API_SERVER_API_H_

#include <google/protobuf/text_format.h>
#include <google/protobuf/stubs/common.h>
#include <string>
#include <vector>
#include "api/protocol/message.pb.h"
#include "api/server/model.h"
#include "api/server/app.h"

class API {
 public:
    /* Process a request from a brut message
     * This will unpack the message and dispatch it to the right API version
     * @param  request request data from the client
     * @param  response response in which the function will write the response
                        to send to the client
     */
    static void process_request(const std::string &request,
        std::string *response);
    // This structure centralize description of NicUpdate informations
    struct NicUpdate {
        std::string id;
        bool has_ip_anti_spoof;
        bool ip_anti_spoof;
        std::vector<app::Ip> ip;
        std::vector<std::string> security_groups;
    };

 protected:
    /* Dispatch a single message processing depending of message version
     * @param req request message
     * @param res response message
     */
    static void dispatch(const proto::Message &req, proto::Message *rep);
    /* Create a NIC and add it in model
     * This method centralize NIC creation for all API versions
     * @param  nic the NIC to create
     * @param  path string path corresponding to the newly created NIC
     * @param  error provide an app::Error object to fill in case of error
     *               can be NULL to ommit it.
     * @return  true if NIC has been created, false otherwise
     */
    static bool action_nic_add(const app::Nic &nic, std::string path,
        app::Error *error);
    /* Update a NIC by replacing provided NIC parameters
     * This method centralize NIC update for all API versions
     * @param  id NIC id to update
     * @param  error provide an app::Error object to fill in case of error
     *               can be NULL to ommit it.
     * @return  true if NIC has been updated, false otherwise
     */
    static bool action_nic_update(const NicUpdate &update,
        app::Error *error);
    /* Delete a NIC and remove it from model
     * This method centralize NIC deletion for all API versions
     * @param  id NIC id to delete
     * @param  error provide an app::Error object to fill in case of error
     *               can be NULL to ommit it.
     * @return  true if NIC has been created, false otherwise
     */
    static bool action_nic_del(std::string id, app::Error *error);
    /* Grab data from a NIC for NIC export
     * This method centralize NIC export for all API versions
     * @param  id NIC id to get data from
     * @param  data where to put NIC data
     * @param  error provide an app::Error object to fill in case of error
     *               can be NULL to ommit it.
     * @return  true if data has been well filled
     */
    static bool action_nic_export(std::string id, std::string *data,
        app::Error *error);
    /* Grab NIC statistics
     * This method centralize NIC statistic collection for all API versions
     * @param  id NIC id to get statistics from
     * @param  in data comming in the NIC
     * @param  out data comming out of the NIC
     * @param  error provide an app::Error object to fill in case of error
     *               can be NULL to ommit it.
     * @return  true if data has been well filled
     */
    static bool action_nic_stats(std::string id, uint64_t *in, uint64_t *out,
        app::Error *error);
    /* Creation a Security Group and add it to the model
     * This method centralize SG creation or replace for all API versions
     * @param  sg security group to create or replace
     * @param  error provide an app::Error object to fill in case of error
     *               can be NULL to ommit it.
     * @return  true if SG has been created or replaced, false otherwise
     */
    static bool action_sg_add(const app::Sg &sg, app::Error *error);
    /* Delete a security group and remove it from model
     * This method centralize SG deletion for all API versions
     * @param  id security group id to delete
     * @param  error provide an app::Error object to fill in case of error
     *               can be NULL to ommit it.
     * @return  true if SG has been delete, false otherwise
     */
    static bool action_sg_del(std::string id, app::Error *error);
    /* Add a rule to a Security Group and add it in the model
     * This method centralize app::Rule creation for all API versions
     * @param  sg_id security group id in which we add a rule
     * @param  rule rule to add in the security group
     * @param  sg security group to create or replace
     * @param  error provide an app::Error object to fill in case of error
     *               can be NULL to ommit it.
     * @return  true if rule has been added, false otherwise
     */
    static bool action_sg_rule_add(std::string sg_id, const app::Rule &rule,
        app::Error *error);
    /* Delete a rule from a Security Group and update the model
     * This method centralize app::Rule deletion for all API versions
     * @param  sg_id security group id in which we delete a rule
     * @param  rule rule to delete in the security group
     * @param  sg security group to create or replace
     * @param  error provide an app::Error object to fill in case of error
     *               can be NULL to ommit it.
     * @return  true if rule has been deleted, false otherwise
     */
    static bool action_sg_rule_del(std::string sg_id, const app::Rule &rule,
        app::Error *error);
    /* Add a member to a Security Group and add it in the model
     * This method centralize Member creation for all API versions
     * @param  sg_id security group id in which we add a member
     * @param  ip member to add in the security group
     * @param  sg security group to create or replace
     * @param  error provide an app::Error object to fill in case of error
     *               can be NULL to ommit it.
     * @return  true if member has been added, false otherwise
     */
    static bool action_sg_member_add(std::string sg_id, const app::Ip &ip,
        app::Error *error);
    /* Delete a member from a Security Group and update the model
     * This method centralize Member deletion for all API versions
     * @param  sg_id security group id in which we delete a member
     * @param  ip member to delete in the security group
     * @param  sg security group to create or replace
     * @param  error provide an app::Error object to fill in case of error
     *               can be NULL to ommit it.
     * @return  true if member has been deleted, false otherwise
     */
    static bool action_sg_member_del(std::string sg_id, const app::Ip &ip,
        app::Error *error);
    /* Build a dot graph representing all connected bricks
     * This method centralize dot representation building
     * @return  string representing the graphic in DOT language
     */
    static std::string action_graph_dot();
    /* Shutdown the program
     * This method centralize program shutdown for all API versions
     */
    static void action_app_quit();
};

class API_0: public API {
 public:
    /* Process a MessageV0 */
    static void process(const MessageV0 &req, MessageV0 *res);

 private:
    /* Methods below construct message from request to response */
    static void nic_add(const MessageV0_Request &req, MessageV0_Response *res);
    static void nic_update(const MessageV0_Request &req,
                           MessageV0_Response *res);
    static void nic_list(const MessageV0_Request &req,
                         MessageV0_Response *res);
    static void nic_details(const MessageV0_Request &req,
                            MessageV0_Response *res);
    static void nic_export(const MessageV0_Request &req,
                           MessageV0_Response *res);
    static void nic_stats(const MessageV0_Request &req,
                          MessageV0_Response *res);
    static void sg_add(const MessageV0_Request &req, MessageV0_Response *res);
    static void sg_del(const MessageV0_Request &req, MessageV0_Response *res);
    static void sg_list(const MessageV0_Request &req, MessageV0_Response *res);
    static void sg_rule_add(const MessageV0_Request &req,
                            MessageV0_Response *res);
    static void sg_rule_del(const MessageV0_Request &req,
                            MessageV0_Response *res);
    static void sg_rule_list(const MessageV0_Request &req,
                             MessageV0_Response *res);
    static void sg_member_add(const MessageV0_Request &req,
                              MessageV0_Response *res);
    static void sg_member_del(const MessageV0_Request &req,
                              MessageV0_Response *res);
    static void sg_member_list(const MessageV0_Request &req,
                               MessageV0_Response *res);
    static void app_status(const MessageV0_Request &req,
                           MessageV0_Response *res);
    static void app_quit(const MessageV0_Request &req,
                         MessageV0_Response *res);
    static void app_config(const MessageV0_Request &req,
                           MessageV0_Response *res);
    /* Methods below pre-format some standard response */
    inline static void build_ok_res(MessageV0_Response *res);
    inline static void build_nok_res(MessageV0_Response *res);
    inline static void build_nok_res(MessageV0_Response *res,
                                     std::string description);
    inline static void build_nok_res(MessageV0_Response *res,
                                     const char *description);
    inline static void build_nok_res(MessageV0_Response *res,
                                     const app::Error &error);
    /* Methods below permits to validate that the content of a Message is
     * is well formated as expected.
     */
    static bool validate_nic(const MessageV0_Nic &nic);
    static bool validate_nic_update(const MessageV0_NicUpdateReq &nic_update);
    static bool validate_sg(const MessageV0_Sg &sg);
    static bool validate_sg_rule(const MessageV0_Rule &rule);
    static bool validate_ip(const std::string &ip);
    static bool validate_mac(const std::string &mac);
    static bool validate_cidr(const MessageV0_Cidr &cidr);
    /* Methods to convert from this message version to object model
     * and reciprocally.
     */
    static bool convert(const app::Nic &nic_model, MessageV0_Nic *nic_message);
    static bool convert(const MessageV0_Nic &nic_message, app::Nic *nic_model);
    static bool convert(const MessageV0_NicUpdateReq &nic_update_message,
                        API::NicUpdate *nic_update_model);
    static bool convert(const app::Sg &sg_model, MessageV0_Sg *sg_message);
    static bool convert(const MessageV0_Sg &sg_message, app::Sg *sg_model);
    static bool convert(const app::Rule &rule_model,
                        MessageV0_Rule *rule_message);
    static bool convert(const MessageV0_Rule &rule_message,
                        app::Rule *rule_model);
    static bool convert(const app::Ip &ip_model, std::string *ip_message);
    static bool convert(const std::string &ip_message, app::Ip *ip_model);
    static bool convert(const app::Mac &mac_model, std::string *mac_message);
    static bool convert(const std::string &mac_message, app::Mac *mac_model);
    static bool convert(const app::Cidr &cidr_model,
                        MessageV0_Cidr *cidr_message);
    static bool convert(const MessageV0_Cidr &cidr_message,
                        app::Cidr *cidr_model);
    static bool convert(const app::Error &error_model,
                        MessageV0_Error *error_message);
};

#endif  // API_SERVER_API_H_
