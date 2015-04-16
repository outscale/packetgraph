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

#ifndef API_SERVER_MODEL_H_
#define API_SERVER_MODEL_H_

#include <string>
#include <vector>
#include <map>
#include <functional>

namespace app {

class Ip {
    typedef enum {
        V4,
        V6,
        NONE
    } type_t;

 public:
    Ip();
    explicit Ip(std::string ip_string);
    type_t type();
    std::string str() const;
    bool bytes(uint8_t *data) const;
    bool operator== (const Ip& a);
    Ip operator= (const std::string &a);
 private:
    std::string ip_;
    type_t type_;
    uint8_t data_[16];
    bool set(std::string i);
};

class Mac {
 public:
    Mac();
    explicit Mac(std::string mac_string);
    std::string str() const;
    bool bytes(uint8_t *data) const;
    bool operator== (const Mac& a);
    Mac operator= (const std::string &a);
 private:
    std::string mac_;
    uint8_t data_[6];
    bool set(std::string mac);
};

struct Cidr {
    Cidr();
    Ip address;
    uint32_t mask_size;
};

struct Nic {
    Nic();
    explicit Nic(std::string id);
    std::string id;
    Mac mac;
    uint32_t vni;
    std::vector<Ip> ip_list;
    std::vector<std::string> security_groups;
    bool ip_anti_spoof;
    std::string sniff_target_nic_id;
};

struct Rule {
    Rule();
    typedef enum {
        INBOUND = 0,
        OUTBOUND = 1,
        DIRECTION_NB
    } direction_t;
    direction_t direction;
    int16_t protocol;
    int32_t port_start;
    int32_t port_end;
    Cidr cidr;
    std::string security_group;
};

struct Sg {
    std::string id;
    std::vector<Ip> members;
    // hash(rule) -> rule
    std::map<std::size_t, Rule> rules;
};

struct Model {
    // SG id -> SG
    std::map<std::string, Sg> security_groups;
    // NIC id -> NIC
    std::map<std::string, Nic> nics;
};

struct Error {
    Error();
    std::string file;
    std::string function;
    std::string description;
    bool has_line;
    uint32_t line;
    bool has_curs_pos;
    uint32_t curs_pos;
    bool has_err_no;
    int32_t err_no;
};

}  // namespace app

// Declatation of hash functions
namespace std {

template<>
struct hash<app::Ip> {
    typedef app::Ip argument_type;
    typedef std::size_t result_type;
    result_type operator()(argument_type const& a) const;
};

template<>
struct hash<app::Mac> {
    typedef app::Mac argument_type;
    typedef std::size_t result_type;
    result_type operator()(argument_type const& a) const;
};

template<>
struct hash<app::Rule> {
    typedef app::Rule argument_type;
    typedef std::size_t result_type;
    result_type operator()(argument_type const& a) const;
};

template<>
struct hash<app::Cidr> {
    typedef app::Cidr argument_type;
    typedef std::size_t result_type;
    result_type operator()(argument_type const& a) const;
};

}  // namespace std
#endif  // API_SERVER_MODEL_H_
