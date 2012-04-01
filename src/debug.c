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
 * @file debug.c
 * @author Christian Partl, Dieter Ladenhauf, Georg Neubauer, Patrick Plaschzug
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "debug.h"
#include "typedef.h"

extern bool b_flag_verbose_;

char* getTypeString(int type)
{
    switch(type)
    {
        case AUTH:
            return "AUTH";
            break;
        case SEC_MEM:
            return "SEC_MEM";
            break;
        case NORMALISE:
            return "NORMALISE";
            break;
        case PARSER:
            return "PARSER";
            break;
        case ENVVAR:
            return "ENVVAR";
            break;
        case CGICALL:
            return "CGICALL";
            break;
        case PATH:
            return "PATH";
            break;
        case STATIC_FILE:
            return "STATIC_FILE";
        case HTTP_RESPONSE:
            return "HTTP_RESPONSE";
        case PIPE:
            return "PIPE";
            break;
        case MAIN:
            return "MAIN";
            break;
		case FILEHANDLING:
			return "FILE";
			break;
        default:
            return "UNSPECIFIED";
    }    
}

void debug(int type, const char *ptr, ...){
  va_list va;
  fprintf(stderr,"%s>> ", getTypeString(type));
  va_start(va, ptr);
  vfprintf(stderr, ptr, va);
  va_end(va);
}

void debugVerbose(int type, const char *ptr, ...){
  va_list va;
  if(!b_flag_verbose_)
    return;
  fprintf(stderr,"%s_VERBOSE>> ", getTypeString(type));
  va_start(va, ptr);
  vfprintf(stderr, ptr, va);
  va_end(va);
}

void debugVerboseHash(int type, const unsigned char* cuca_hash, int i_hash_len, const char* cca_ptr, ...)
{
    va_list va;
    int i = 0;
    if(!b_flag_verbose_)
        return;
    
    fprintf(stderr,"%s_VERBOSE>> ", getTypeString(type));
    
    va_start(va, cca_ptr);
    vfprintf(stderr, cca_ptr, va);
    va_end(va);
    
    fprintf(stderr, " Hash: ");
    
    for (i = 0; i < i_hash_len; i++)
    {
            fprintf(stderr, "%x", cuca_hash[i]);
    }
    
    fprintf(stderr, "\n");
    
}
