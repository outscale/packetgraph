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

#endif  // API_SERVER_API_H_
