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

#ifndef API_CLIENT_H
#define API_CLIENT_H

#include <glib.h>

struct Options
{
    Options();
    bool parse(int argc, char **argv);
    bool missing();
    gchar *endpoint;
    gchar *input;
    gchar *output;
    gboolean std_out;
    gchar *proto;
    gboolean revision;
    gboolean verbose;
};


#endif
