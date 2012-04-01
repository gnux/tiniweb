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
 * @file staticfile.h
 * @author Christian Partl
 */

#ifndef __STATIC_FILE_H__
#define __STATIC_FILE_H__

/**
 * Sends a HTTP response including the specified static file.
 *
 * @param cp_path Path (including filename) to the file
 */
void processStaticFile(const char* path);

/**
 * Writes the file of a source file descriptor to the destination file descriptor.
 *
 * @param i_src_fd File descriptor of the source
 * @param i_dest_fd File descriptor of the destination
 * @return EXIT_SUCCESS if no problem occurred, EXIT_FAILURE otherwise.
 */
int writeFileTo(int i_src_fd, int i_dest_fd);

#endif

