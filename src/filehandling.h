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
 * tiniweb
 * 
 * @file filehandling.h
 * @author Plaschzug, Partl, Ladenhauf, Neubauer
 */

#ifndef __FILE_HANDLING_H__
#define __FILE_HANDLING_H__

/**
* This function retrieves an complete header
* @param fd filedescriptor of input file
* @param timeout per byte
*/
char* retrieveHeader(int fd, int timeout);

/**
* This function retrieves num bytes form file, it does polling for each byte
* @param fd filedescriptor of input file
* @param timeout per byte
* @param num number of bytes to read
*/
ssize_t pollAndRead(int fd, int timeout, char* cp_buffer, ssize_t num);

#endif
