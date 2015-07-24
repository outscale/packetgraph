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

#ifndef _BRICKS_BRICK_FIREWALL_H_
#define _BRICKS_BRICK_FIREWALL_H_

#include <packetgraph/common.h>
#include <packetgraph/utils/errors.h>

/**
 * Create a new firewall brick
 *
 * @param	name name of the brick
 * @param	west_max maximum of links you can connect on the west side
 * @param	east_max maximum of links you can connect on the east side
 * @param	errp is set in case of an error
 * @return	a pointer to a brick structure, on success, 0 on error
 */
struct brick *firewall_new(const char *name, uint32_t west_max,
			uint32_t east_max, struct switch_error **errp);

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
 * @return	0 if the rule has been correctly built and added the the brick
 *		1 otherwise and errp is set.
 */
int firewall_rule_add(struct brick *brick, const char *filter, enum side dir,
		      int stateful, struct switch_error **errp);

/**
 * Flush all rules of the firewall.
 * Note that the flush won't be effective, you will need to call
 * girewall_reload() before.
 *
 * @param	brick pointer to the firewall brick
 */
void firewall_rule_flush(struct brick *brick);

/**
 * Reload firewall rules.
 * All rules added in the firewall will be loaded.
 * Old rules won't be effective anymore.
 *
 * @param	brick pointer to the firewall brick
 * @param	errp is set in case of an error
 * @return	0 if the firewall has reloaded correctly
 *		1 otherwise and errp is set.
 */
int firewall_reload(struct brick *brick, struct switch_error **errp);

#endif  /* _BRICKS_BRICK_FIREWALL_H_ */
