/* Copyright 2015 Outscale SAS
 *
 * This file is part of Packetgraph.
 *
 * Packetgraph is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as published
 * by the Free Software Foundation.
 *
 * Packetgraph is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Packetgraph.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _PG_VTEP_H
#define _PG_VTEP_H

#include <packetgraph/packetgraph.h>
#include <packetgraph/errors.h>

#define PG_VTEP_DST_PORT 4789

#define PG_VTEP_GENERIC_IPV6(callback) uint8_t [16] : callback,	\
		uint8_t * : callback

enum pg_vtep_flags {
	PG_VTEP_NONE = 0,
	/* modify the packets instead of copy it */
	PG_VTEP_NO_COPY = 1,
	/* don't check if the VNI has the coresponding mac,
	 * when incoming packets, just forward it */
	PG_VTEP_NO_INNERMAC_CHECK = 2,
	PG_VTEP_FORCE_UPD_IPV6_CHECKSUM = 4,
	PG_VTEP_ALL_OPTI = PG_VTEP_NO_COPY | PG_VTEP_NO_INNERMAC_CHECK
};

struct ether_addr;

/**
 * Get MAC address of the vtep
 * @param   brick a pointer to a vtep brick
 * @return  mac address of the vtep
 */
struct ether_addr *pg_vtep_get_mac(const struct pg_brick *brick);

/**
 * Cleanup mac that have timeout
 * This operation iterate over every mac
 * It's slow, even with an empty mac table
 * @param brick a pointer to a vtep brick
 * @param port_id the port id (so the index of the connected brick)
 */
void pg_vtep4_cleanup_mac(struct pg_brick *brick, int port_id);

/**
 * Cleanup mac that have timeout
 * This operation iterate over every mac
 * It's slow, even with an empty mac table
 * @param brick a pointer to a vtep brick
 * @param port_id the port id (so the index of the connected brick)
 */
void pg_vtep6_cleanup_mac(struct pg_brick *brick, int port_id);

/**
 * Add a VNI to the VTEP
 *
 * @param   brick the brick we are working on
 * @param   neighbor a brick connected to the VTEP port
 * @param   vni the VNI to add, must be < 2^24
 * @param   multicast_ip the multicast ip to associate to the VNI
 * @param   errp an error pointer
 * @return  0 on success, -1 on error
 */
int pg_vtep4_add_vni(struct pg_brick *brick,
		     struct pg_brick *neighbor,
		     uint32_t vni, uint32_t multicast_ip,
		     struct pg_error **errp);

/**
 * Add a VNI to the VTEP
 *
 * @param   brick the brick we are working on
 * @param   neighbor a brick connected to the VTEP port
 * @param   vni the VNI to add, must be < 2^24
 * @param   multicast_ip the multicast ip to associate to the VNI
 * @param   errp an error pointer
 * @return  0 on success, -1 on error
 */
int pg_vtep6_add_vni(struct pg_brick *brick, struct pg_brick *neighbor,
		     uint32_t vni, uint8_t *multicast_ip,
		     struct pg_error **errp);

#ifndef __cplusplus

/**
 * Add a VNI to the VTEP
 *
 * @param   brick the brick we are working on
 * @param   neighbor a brick connected to the VTEP port
 * @param   vni the VNI to add, must be < 2^24
 * @param   multicast_ip the multicast ip to associate to the VNI
 *          if an 'uint32_t' is given then an ipv4 is add,
 *          if an 'uint8_t *' is given then an ipv6 is add
 * @param   errp an error pointer
 */
#define pg_vtep_add_vni(brick, neighbor, vni, multicast_ip, errp)	\
	(_Generic((multicast_ip), uint32_t : pg_vtep4_add_vni,		\
		  int32_t : pg_vtep4_add_vni,				\
		  PG_VTEP_GENERIC_IPV6(pg_vtep6_add_vni))		\
	 (brick, neighbor, vni, multicast_ip, errp))

#else

extern "C++" {
namespace {
inline int pg_vtep_add_vni(struct pg_brick *brick,
			   struct pg_brick *neighbor,
			   uint32_t vni, uint32_t multicast_ip,
			   struct pg_error **errp)
{
	return pg_vtep4_add_vni(brick, neighbor, vni, multicast_ip, errp);
}

inline int pg_vtep_add_vni(struct pg_brick *brick,
			   struct pg_brick *neighbor,
			   uint32_t vni, uint8_t *multicast_ip,
			   struct pg_error **errp)
{
	return pg_vtep6_add_vni(brick, neighbor, vni, multicast_ip, errp);
}
}
}

#endif

/**
 * Add a MAC to the VTEP VNI
 *
 * @param   brick the brick we are working on
 * @param   vni the vni on which you want to add mac
 * @param   mac the mac
 * @param   errp an error pointer
 * @return  0 on success, -1 on error
 */
int pg_vtep_add_mac(struct pg_brick *brick, uint32_t vni,
		    struct ether_addr *mac, struct pg_error **errp);

/**
 * Like what the function name say
 * @param   brick the brick we are working on
 * @param   vni the vni on which you want to add mac
 * @param   mac the mac
 * @param   errp an error pointer
 * @return  0 on success, -1 on error
 */
int pg_vtep_unset_mac(struct pg_brick *brick, uint32_t vni,
		      struct ether_addr *mac, struct pg_error **errp);

/**
 * Create a new vtep
 *
 * @param   name brick name
 * @param   max maximum number of connections
 * @param   output side where packets are encapsuled in VXLAN
 * @param   ip vtep ip
 * @param   mac vtep mac
 * @param   udp_dst_port udp destination port of vxlan packets (in cpu endinaness)
 *          default port is PG_VTEP_DST_PORT
 * @param   flag vtep flags (see pg_vtep_flags)
 * @param   errp error pointer set on error
 * @return  pointer to a brick structure on success, NULL on error
 */
PG_WARN_UNUSED
struct pg_brick *pg_vtep6_new(const char *name, uint32_t max,
			      enum pg_side output, uint8_t *ip,
			      struct ether_addr mac, uint16_t udp_dst_port,
			      int flag, struct pg_error **errp);

/**
 * Create a new vtep
 *
 * @param   name brick name
 * @param   max maximum number of connections
 * @param   output side where packets are encapsuled in VXLAN
 * @param   ip vtep ip
 * @param   mac vtep mac
 * @param   udp_dst_port udp destination port of vxlan packets (in cpu endinaness)
 *          default port is PG_VTEP_DST_PORT
 * @param   flag vtep flags (see pg_vtep_flags)
 * @param   errp error pointer set on error
 * @return  pointer to a brick structure on success, NULL on error
 */
PG_WARN_UNUSED
struct pg_brick *pg_vtep4_new(const char *name, uint32_t max,
			      enum pg_side output, uint32_t ip,
			      struct ether_addr mac, uint16_t udp_dst_port,
			      int flag, struct pg_error **errp);

/**
 * Same as pg_vtep_new but use a string as ip
 */
struct pg_brick *pg_vtep_new_by_string(const char *name, uint32_t max,
				       enum pg_side output, const char *ip,
				       struct ether_addr mac,
				       uint16_t udp_dst_port,
				       int flag, struct pg_error **errp);

#ifndef __cplusplus

/**
 * Create a new vtep
 *
 * @param   name brick name
 * @param   max maximum number of connections
 * @param   output side where packets are encapsuled in VXLAN
 * @param   ip if an 'uint32_t' is given then an ipv4 vtep is created,
 *          if an 'uint8_t *' is given then an ipv6 vtep is created
 * @param   mac vtep mac
 * @param   udp_dst_port udp destination port of vxlan packets (in cpu endinaness)
 *          default port is PG_VTEP_DST_PORT
 * @param   flags vtep flags (see pg_vtep_flags)
 * @param   errp error pointer set on error
 * @return  pointer to a brick structure on success, NULL on error
 */
#define pg_vtep_new(name, max, output, ip, mac, udp_dst_port, flags, errp) \
	(_Generic((ip), uint32_t : pg_vtep4_new,			\
		  int32_t : pg_vtep4_new,				\
		  PG_VTEP_GENERIC_IPV6(pg_vtep6_new))			\
	 (name, max, output, ip, mac, udp_dst_port, flags, errp))

#else

extern "C++" {

#include <netinet/ether.h>

namespace {
inline struct pg_brick *pg_vtep_new(const char *name, uint32_t max,
				    enum pg_side output, uint32_t ip,
				    struct ether_addr mac,
				    uint16_t udp_dst_port,
				    int flag, struct pg_error **errp)
{
	return pg_vtep4_new(name, max, output, ip, mac, udp_dst_port,
			    flag, errp);
}

inline struct pg_brick *pg_vtep_new(const char *name, uint32_t max,
				    enum pg_side output, uint8_t *ip,
				    struct ether_addr mac,
				    uint16_t udp_dst_port,
				    int flag, struct pg_error **errp)
{
	return pg_vtep6_new(name, max, output, ip, mac, udp_dst_port,
			    flag, errp);
}
}
}

#endif /* __cplusplus */

#endif /* _PG_VTEP_H */
