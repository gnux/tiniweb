/** tiniweb
 * @file path.h
 * @author Dieter Ladenhauf
 */

#ifndef __PATH_H__
#define __PATH_H__

#include <sys/types.h>
#include "typedef.h"


bool performPathChecking(char** cpp_path_cgi, char** cpp_path_web);

/**
 * Checks if a path/file exists
 *
 * @param ca_path the path/file
 * @return TRUE if the path/file exists FALSE if not
 */
bool checkPath(char* ca_path);

bool checkIfCGIDirContainsWebDir(char* ca_path_cgi, char* ca_path_web);

void deleteCyclesFromPath(char** cpp_path);

bool isAbsolutePath(char * cp_path);

void constructAbsolutePath(char** cpp_path);

void testPathChecking();


#endif