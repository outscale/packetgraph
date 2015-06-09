/* Copyright 2014 Nodalink EURL
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

#include <glib.h>

#include "utils/config.h"
#include "bricks/brick-int.h"

struct brick_config *diode_config_new(const char *name, uint32_t west_max,
				      uint32_t east_max, enum side output)
{
	struct brick_config *config = g_new0(struct brick_config, 1);
	struct diode_config *diode_config = g_new0(struct diode_config, 1);

	diode_config->output = output;
	config->diode = diode_config;
	return brick_config_init(config, name, west_max, east_max);
}

struct brick_config *packetsgen_config_new(const char *name, uint32_t west_max,
				      uint32_t east_max, enum side output,
				      struct rte_mbuf **packets,
				      uint16_t packets_nb)
{
	struct brick_config *config = g_new0(struct brick_config, 1);
	struct packetsgen_config *pc = g_new0(struct packetsgen_config, 1);

	pc->output = output;
	pc->packets = packets;
	pc->packets_nb = packets_nb;
	config->packetsgen = pc;
	return brick_config_init(config, name, west_max, east_max);
}

struct brick_config *brick_config_init(struct brick_config *config,
				       const char *name,
				       uint32_t west_max,
				       uint32_t east_max)
{
	config->name = g_strdup(name);
	config->west_max = west_max;
	config->east_max = east_max;
	return config;
}

struct brick_config *brick_config_new(const char *name, uint32_t west_max,
				      uint32_t east_max)
{
	struct brick_config *config = g_new0(struct brick_config, 1);

	return brick_config_init(config, name, west_max, east_max);
}

struct brick_config *vhost_config_new(const char *name, uint32_t west_max,
				      uint32_t east_max,
				      enum side output)
{
	struct brick_config *config = g_new0(struct brick_config, 1);
	struct vhost_config *vhost_config = g_new0(struct vhost_config, 1);

	vhost_config->output = output;
	config->vhost = vhost_config;
	return brick_config_init(config, name, west_max, east_max);
}

struct brick_config *nic_config_new(const char *name, uint32_t west_max,
				    uint32_t east_max,
				    enum side output,
				    const char *ifname,
				    uint8_t portid)
{
	struct brick_config *config = g_new0(struct brick_config, 1);
	struct nic_config *nic_config = g_new0(struct nic_config, 1);

	if (ifname) {
		nic_config->ifname = g_strdup(ifname);
	} else {
		nic_config->ifname = NULL;
		nic_config->portid = portid;
	}
	nic_config->output = output;
	config->nic = nic_config;
	return brick_config_init(config, name, west_max, east_max);
}

void brick_config_free(struct brick_config *config)
{
	g_free(config->vhost);
	if (config->nic) {
		g_free(config->nic->ifname);
		g_free(config->nic);
	}
	g_free(config->diode);
	g_free(config->packetsgen);
	g_free(config->name);
	g_free(config);
}
