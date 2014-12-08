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

#ifndef _UTILS_CONFIG_H_
#define _UTILS_CONFIG_H_

#include "config.pb-c.h"

#define nop_config	_NopConfig
#define collect_config	_CollectConfig
#define brick_config	_BrickConfig

struct brick_config *brick_config_new(const char *name, uint32_t west_max,
				      uint32_t east_max);

void brick_config_free(struct brick_config *config);

#endif
