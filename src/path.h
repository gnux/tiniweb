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
 * @file path.h
 * @author Dieter Ladenhauf
 */

#ifndef __PATH_H__
#define __PATH_H__

#include <sys/types.h>
#include "typedef.h"

/**
 * Sorts the path in a double array. The result will be e.g:
 *
 *   cpp_path[0] = "Juhu/"
 *   cpp_path[0][0] = 'J'
 *
 * @param cp_path the path to sort
 * @param cppp_sorted_path the sorted path to be stored in
 * @return number of folders
 */
int getSortedPath(char* cp_path, char*** cppp_sorted_path);

/**
 * Function to free the allocated memory from the sorted path above
 *
 * @param cpp_path the path to be freed
 * @param i_num_folders the number of folders (cp_path[i_num_folders][x])
 */
void freeSortedPath(char** cpp_path, int i_num_folders);

/**
 * Checks the two given commandline paths
 *
 * @param cpp_path_cgi path of the cgi directory
 * @param cpp_path_web path of the web dir
 * @return TRUE if everything worked, FALSE if not
 */
bool performPathChecking(char** cpp_path_cgi, char** cpp_path_web);

/**
 * Uses the function realpath(...) to construct the real path (without '..', '.' 
 *  and '///')
 *
 * @param cp_path the path to convert
 * @return TRUE if it worked, FALSE if not
 */
bool convertToRealPath(char** cp_path);

/**
 * Checks if a path/file exists
 *
 * @param ca_path the path/file
 * @return TRUE if the path/file exists FALSE if not
 */
bool checkPath(char* ca_path);

/**
 * Checks if a path exists and if it is a folder
 *
 * @param ca_path the path/file
 * @return TRUE if the path/file exists FALSE if not
 */
bool checkCommadlinePath(char* cp_path);

/**
 * Checks if a path/file exists and if the expected ressource is a regular file
 *
 * @param ca_path the path/file
 * @return TRUE if the path/file exists FALSE if not
 */
bool checkRequestPath(char* cp_path);

/**
 * Checks if the first Directory contains the second Directory. This can be useful
 * whrn checking, if the cgi dir contains the web dir
 *
 * @param cpp_path_cgi the first directory
 * @param cpp_path_web the second directory
 * @return TRUE if the first Dir contains the second dir, FALSE if not
 */
bool checkIfFirstDirContainsSecondDir(char* ca_path_cgi, char* ca_path_web);

/**
 * Deletes Cycles like 'foo/..' from the path. The '.' well be deleted too
 *   This Function is not used in our implementation any more. It is still in the
 *   Code, because of possible future extensions.
 * @param cpp_path the path from which teh cycles should be removed.
 */
void deleteCyclesFromPath(char** cpp_path);

/**
 * Checks if the path is an absolute path (starts with '/')
 *
 * @param cpp_path the path to check
 * @return TRUE if it is an absolute path, false if not
 */
bool isAbsolutePath(char * cp_path);

/**
 * Constructs the absolute path
 *
 * @param cpp_path the relative path, which becomes an absulute path
 */
void constructAbsolutePath(char** cpp_path);

/**
 * Maps the request path to the local file structure
 *
 * @param cpp_final_path mapped path to the requested resource is going to be stored here
 * @param cb_static boolean to be stored in. Shows if the request is static or not
 * @return TRUE in case of success, FALSE in case of an error
 */
bool mapRequestPath(char** cpp_final_path, bool *cb_static);

#endif
