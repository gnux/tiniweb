/** 
 * Copyright 2009-2012
 * 
 * Dieter Ladenhauf
 * Georg Neubauer
 * Christian Partl
 * Patrick Plaschzug
 * 
 * This file is part of Wunderwuzzi.
 * 
 * Wunderwuzzi is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Wunderwuzzi is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Wunderwuzzi. If not, see <http://www.gnu.org/licenses/>.
 * 
 * -------------------------------------------------------------------
 * 
 * 
 * @file debug.h
 * @author Christian Partl, Dieter Ladenhauf, Georg Neubauer, Patrick Plaschzug
 */

#ifndef __DEBUG_H__
#define __DEBUG_H__

//Do NOT include stdlib!

#include "typedef.h"

static const enum SCE_DEBUG_TYPES
{
    MAIN,
    SEC_MEM,
    AUTH,
    NORMALISE,
    PARSER,
    ENVVAR,
    CGICALL,
    STATIC_FILE,
    HTTP_RESPONSE,
    PIPE,
    PATH,
	FILEHANDLING
} SCE_DEBUG_TYPE;

void debug(int type, const char *ptr, ...);
void debugVerbose(int type, const char *ptr, ...);

/**
 * Special debug method, that prints a hash value to stderr too.
 * 
 * @param i_type type of the debug message
 * @param cuca_hash the hash to print
 * @param i_hash_len the length of thee hash
 * @param cca_ptr the normal message
 */
void debugVerboseHash(int type, const unsigned char* cuca_hash, int i_hash_len, const char* cca_ptr, ...);

#endif

