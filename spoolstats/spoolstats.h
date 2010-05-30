/*
 * This file is part of spoolstats.
 * Copyright (C) 2010 Richard Kettlewell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */
#ifndef SPOOLSTATS_H
#define SPOOLSTATS_H

#include <config.h>

#include <set>
#include <map>
#include <string>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstring>
#include <cerrno>

#include "utils.h"
#include "cpputils.h"
#include "Article.h"
#include "Bucket.h"
#include "SenderCountingBucket.h"
#include "AllGroups.h"
#include "Hierarchy.h"
#include "Group.h"
#include "Conf.h"
#include "HTML.h"

extern "C" const char sorttable[];
extern "C" const char css[];

#endif /* SPOOLSTATS_H */

/*
Local Variables:
mode:c++
c-basic-offset:2
comment-column:40
fill-column:79
indent-tabs-mode:nil
End:
*/
