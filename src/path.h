/** tiniweb
 * @file path.h
 * @author Dieter Ladenhauf
 */

#ifndef __PATH_H__
#define __PATH_H__

#include <sys/types.h>
#include "typedef.h"

/**
 * 
 *
 * @param cp_path 
 * @param cppp_sorted_path 
 * @return 
 */
int getSortedPath(char* cp_path, char*** cppp_sorted_path);

/**
 * 
 *
 * @param cp_path 
 * @param i_num_folders 
 * @return 
 */
void freeSortedPath(char** cpp_path, int i_num_folders);

/**
 * 
 *
 * @param cpp_path_cgi 
 * @param cpp_path_web 
 * @return 
 */
bool performPathChecking(char** cpp_path_cgi, char** cpp_path_web);

/**
 * Checks if a path/file exists
 *
 * @param ca_path the path/file
 * @return TRUE if the path/file exists FALSE if not
 */
bool checkPath(char* ca_path);

/**
 * 
 *
 * @param cpp_path_cgi 
 * @param cpp_path_web 
 * @return 
 */
bool checkIfCGIDirContainsWebDir(char* ca_path_cgi, char* ca_path_web);

/**
 * 
 *
 * @param cpp_path
 */
void deleteCyclesFromPath(char** cpp_path);

/**
 * 
 *
 * @param cpp_path
 * @return 
 */
bool isAbsolutePath(char * cp_path);

/**
 * 
 *
 * @param cpp_path
 * @return 
 */
void constructAbsolutePath(char** cpp_path);

// TODO remove if not needed any more
void testPathChecking();


#endif
