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

#ifndef _PG_FIREWALL_H
#define _PG_FIREWALL_H

#include <packetgraph/common.h>
#include <packetgraph/errors.h>

#define PG_NONE 0
#define PG_NO_CONN_WORKER 0x00000001

/**
 * Create a new firewall brick.
 * This function should be call in the same thread than the one
 * which process packets.
 *
 * @param	name name of the brick
 * @param	west_max maximum of links you can connect on the west side
 * @param	east_max maximum of links you can connect on the east side
 * @param	flags pass PG_NO_CONN_WORKER if you don't want NPF to spawn a
 *              separated thread to run garbage collector to clean old
 *              connexions. If you pass this flag, then you will be responsible
 *              of calling pg_firewall_gc.
 * @param	errp is set in case of an error
 * @return	a pointer to a brick structure on success, NULL on error
 */
PG_WARN_UNUSED
struct pg_brick *pg_firewall_new(const char *name, uint32_t west_max,
				 uint32_t east_max, uint64_t flags,
				 struct pg_error **errp);

/**
 * Add a new rule in the firewall.
 * Note that the rule won't be effective, you will need to call
 * firewall_reload() before. Offcourse, you can call firewall_rule_add
 * several times before firewall_reload().
 *
 * @param	brick pointer to the firewall brick
 * @param	filter a pcap-filter string to apply, if a NULL string is
 *		passed, it will allow all traffic.
 * @param	side side where the filter will occurs, you can use SIDE_MAX
 *		to specify both sides.
 * @param	errp is set in case of an error
 * @return	0 if the rule has been correctly built and added
 *		-1 on error and errp is set.
 */
int pg_firewall_rule_add(struct pg_brick *brick, const char *filter,
			 enum pg_side dir, int stateful,
			 struct pg_error **errp);

/**
 * Manually call the firewall garbage collector.
 * NPF firewall tracks all connexions when stateful.
 * To clean old connexions, we must manually call the connexion garbage
 * collection when PG_NO_CONN_WORKER flag shas not been set at brick creation.
 *
 * @param	brick pointer to the firewall brick
 */
void pg_firewall_gc(struct pg_brick *brick);

/**
 * Flush all rules of the firewall.
 * Note that the flush won't be effective, you will need to call
 * firewall_reload() before.
 *
 * @param	brick pointer to the firewall brick
 */
void pg_firewall_rule_flush(struct pg_brick *brick);

/**
 * Reload firewall rules.
 * All rules added in the firewall will be loaded.
 * Old rules won't be effective anymore.
 *
 * @param	brick pointer to the firewall brick
 * @param	errp is set in case of an error
 * @return	0 if the firewall has reloaded correctly
 *		-1 otherwise and errp is set.
 */
int pg_firewall_reload(struct pg_brick *brick, struct pg_error **errp);

#endif  /* _PG_FIREWALL_H */
