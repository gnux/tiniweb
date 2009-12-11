
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <errno.h>

#include "path.h"
#include "debug.h"
#include "normalize.h"
#include "secmem.h"
#include "secstring.h"
#include "httpresponse.h"

extern char *scp_web_dir_;
extern char *scp_cgi_dir_;
extern http_request *http_request_;
const int CI_MAX_PATH_LEN = 100;

int getSortedPath(char* cp_path, char*** cppp_sorted_path)
{
    
    /**
     *  Store the path into the sorted cpp_path. The result will be e.g:
     *
     *   cpp_path[0] = "Juhu"
     *   cpp_path[0][0] = 'J'
     */
    
    int i_path_len = strlen(cp_path);
    int i_num_folders = 0;
    int i_start = 0;
    
    for (int i_end = 0; i_end < i_path_len; i_end++)
    {
        
        if (cp_path[i_end] == '/' || i_end == i_path_len - 1)
        {
            int i_num_chars = i_end - i_start + 1;

            if (i_num_folders == 0)
                (*cppp_sorted_path) = (char**) secMalloc((i_num_folders + 1) * sizeof(char*));
            else
                (*cppp_sorted_path) = (char**) secRealloc((*cppp_sorted_path), (i_num_folders + 1) * sizeof(char*));
            
            (*cppp_sorted_path)[i_num_folders] = (char*) secMalloc((i_num_chars + 1) * sizeof(char));
            
            // Write all chars into new sorted array
            strncpy((*cppp_sorted_path)[i_num_folders], cp_path + i_start, i_num_chars);
            (*cppp_sorted_path)[i_num_folders][i_num_chars] = '\0';
            
            i_start = i_end + 1;
            i_num_folders++;
        }
    }
    
    return i_num_folders;
}

void freeSortedPath(char** cpp_path, int i_num_folders)
{
    for (int i_current_folder = 0; i_current_folder < i_num_folders; i_current_folder++)
        secFree(&(cpp_path[i_current_folder]));
    secFree(cpp_path);
}

bool performPathChecking(char** cpp_path_cgi, char** cpp_path_web)
{
    constructAbsolutePath(cpp_path_cgi);
    constructAbsolutePath(cpp_path_web);
    
    if (convertToRealPath(cpp_path_cgi) == FALSE || convertToRealPath(cpp_path_web) == FALSE)
    {
        return FALSE;
    }
    
    if (checkCommadlinePath(*cpp_path_cgi) == FALSE || checkCommadlinePath(*cpp_path_web) == FALSE)
    {
        return FALSE;
    }
    
    if (checkIfFirstDirContainsSecondDir(*cpp_path_cgi, *cpp_path_web) == TRUE)
    {
        return FALSE;
    }
    
    debug(PATH, "Success: Path checking finished! No error ocurred!\n");
    return TRUE;
}

bool convertToRealPath(char** cp_path)
{
    char* ca_path_buffer = secCalloc(MAXPATHLEN + 1, sizeof(char));
    char* cp_realpath_result = NULL;
    
    /**
     *  Construct the final path (without '..' '.' and '//')
     */
    cp_realpath_result = realpath(*cp_path, ca_path_buffer);
    if ( cp_realpath_result == NULL || cp_realpath_result != ca_path_buffer)
    {
        debugVerbose(PATH, "ERROR, Path to File/Directory: '%s' could not be resolved!\n", *cp_path);
        return FALSE;
    }
    *cp_path = ca_path_buffer;
    return TRUE;
}

bool checkPath(char* ca_path)
{
    struct stat buffer;
    int i_result = lstat(ca_path, &buffer);
    
    if (i_result == 0)
    {
        debugVerbose(PATH, "Success: Directory Checked: Directory/File %s is valid!\n", ca_path);
        return TRUE;
    }

    debugVerbose(PATH, "ERROR: Directory Checked: Directory/File %s is NOT valid!\n", ca_path);
    return FALSE;
}

bool checkCommadlinePath(char* ca_path)
{
    struct stat buffer;
    int i_result = lstat(ca_path, &buffer);
    
    if (i_result == 0)
    {
        debugVerbose(PATH, "Success: Directory Checked: Directory/File %s is valid!\n", ca_path);
        
        if ( (buffer.st_mode & S_IFMT) != S_IFDIR )
        {
            debugVerbose(PATH, "ERROR: The Requested ressource is no directory!\n");
            return FALSE;
        }
        return TRUE;
    }

    debugVerbose(PATH, "ERROR: Directory Checked: Directory/File %s is NOT valid!\n", ca_path);
    return FALSE;
}

bool checkRequestPath(char* cp_path)
{
    struct stat buffer;
    int i_result = lstat(cp_path, &buffer);
    
    if (i_result == 0)
    {
        debugVerbose(PATH, "Success: Directory Checked: Directory/File %s is valid!\n", cp_path);
        
        // Is the file a regular file?
        if ( (buffer.st_mode & S_IFMT) != S_IFREG)
        {
            // Is it a folder?
//             if ( (buffer.st_mode & S_IFMT) == S_IFDIR )
//             {
//                 /**
//                  *  Now we have to map to index.html in that folder!
//                  *  If it exists, continue with the mapped path
//                  */                
//                 
//                 debugVerbose(PATH, "INFO: The Requested ressource is a folder! Trying to map to index.html\n");
//                 strAppend(&cp_path, "/index.html");
//                 
//                 struct stat index_path_buffer;
//                 if (lstat(cp_path, &index_path_buffer) == 0)
//                     return TRUE;
//                 
//             }
            debugVerbose(PATH, "ERROR: The Requested ressource is no file!\n");
            return FALSE;
        }
    
        return TRUE;
    }

    debugVerbose(PATH, "ERROR: Directory Checked: Directory/File %s is NOT valid!\n", cp_path);   
    return FALSE;
}

bool checkIfFirstDirContainsSecondDir(char* ca_path_cgi, char* ca_path_web)
{
    int i_path_cgi_len = strlen(ca_path_cgi);
    int i_path_web_len = strlen(ca_path_web);
    int i_shortest_path_len = 0;
    int i = 0;
    bool b_cgi_dir_contains_web_dir = FALSE;
    
    if (i_path_web_len >= i_path_cgi_len)
    {
        i_shortest_path_len = i_path_cgi_len;
    
        for (i = 0; i < i_shortest_path_len; i++)
        {
            int i_result = strncmp(ca_path_cgi, ca_path_web, i);
            if (i_result == 0)
            {
                b_cgi_dir_contains_web_dir = TRUE;
            }
            else
            {
                b_cgi_dir_contains_web_dir = FALSE;
                break;
            }
        }
    }
    
    if (!b_cgi_dir_contains_web_dir)
        debugVerbose(PATH, "Success: First-Dir: '%s' does not contain Second-Dir: '%s'!\n", ca_path_cgi, ca_path_web);
    else
        debugVerbose(PATH, "Attention: First-Dir: '%s' contains Second-Dir: '%s'!\n", ca_path_cgi, ca_path_web);
    
    return b_cgi_dir_contains_web_dir;
}

void deleteCyclesFromPath(char** cpp_path_to_check)
{
    char** cpp_path = NULL;
    char* cp_result_path = NULL;
    int i_num_folders = 0;

    i_num_folders = getSortedPath((*cpp_path_to_check), &cpp_path);

    /**
     *  Perform the checking if '../' or './' are in the path
     *   if there is a '..' search for the folder before and delete it
     *   if there is a '.' delete it
     */
    for (int i_current_folder = 0; i_current_folder < i_num_folders; i_current_folder++)
    {
        // handle the '..'
        if (( strlen(cpp_path[i_current_folder]) >= 3 && strncmp(cpp_path[i_current_folder], "../", 3) == 0) ||
            ( strlen(cpp_path[i_current_folder]) >= 2 && strncmp(cpp_path[i_current_folder], "..", 2) == 0 ) )
        {
            cpp_path[i_current_folder][0] = '\0';
            if (i_current_folder > 0)
            {

                // Check recursive for the last folder
                for (int i_folder_reverse = i_current_folder - 1; i_folder_reverse >= 0; i_folder_reverse--)
                {
                    
                    // If first folder just contains '/' do nothing
                    if (i_folder_reverse == 0 && cpp_path[i_folder_reverse][0] == '/' && 
                        strlen(cpp_path[i_folder_reverse]) == 1)
                    {
                        break;
                    }
                    
                    // Erase the Folder before the '..'
                    if (cpp_path[i_folder_reverse][0] != '\0')
                    {
                        cpp_path[i_folder_reverse][0] = '\0';
                        break;
                    }
                }
            }
        }
        
        // handle the '.'
        else if ( ( strlen(cpp_path[i_current_folder]) >= 2 && strncmp(cpp_path[i_current_folder], "./", 2) == 0 ) || 
                  ( cpp_path[i_current_folder][0] == '.' && cpp_path[i_current_folder][1] == '\0' ) )
        {
            cpp_path[i_current_folder][0] = '\0';
        }
        
        // handle the '/'
        else if ( strlen(cpp_path[i_current_folder]) >= 1 && 
                  strncmp(cpp_path[i_current_folder], "/", 1) == 0 && 
                  i_current_folder != 0 )
        {
            cpp_path[i_current_folder][0] = '\0';
        }
    }
    
    /**
     *  Store the result into the resultstring.
     */
    for (int i_current_folder = 0; i_current_folder < i_num_folders; i_current_folder++)
    {
        if (cpp_path[i_current_folder][0] != '\0')
        {
            strAppend(&cp_result_path, cpp_path[i_current_folder]);
        }
    }
    
    /**
     *  Free Allocated Memory
     */
    freeSortedPath(cpp_path, i_num_folders);
    
    (*cpp_path_to_check) = cp_result_path;
    debugVerbose(PATH, "Removed All cycles from %s\n", (*cpp_path_to_check));
}

bool isAbsolutePath(char * cp_path)
{
    int i_path_len = strlen(cp_path);
    if (i_path_len >= 1)
    {
        if (cp_path[0] == '/')
            return TRUE;
    }
    
    return FALSE;
}

void constructAbsolutePath(char** cpp_path)
{
    if (!isAbsolutePath(*cpp_path))
    {
        char* cp_buffer = NULL;
        int i_buffer_len = 20;
        cp_buffer = secCalloc(i_buffer_len, sizeof(char));
        char* cp_result = getcwd(cp_buffer, i_buffer_len);
        
        while (cp_result == NULL)
        {
            i_buffer_len += 1;
            cp_buffer = secRealloc(cp_buffer, i_buffer_len * sizeof(char));
            cp_result = getcwd(cp_buffer, i_buffer_len);
        }
        
        strAppend(&cp_result, "/");
        strAppend(&cp_result, *cpp_path);
        secFree(cpp_path);
        secFree(*cpp_path);
        (*cpp_path) = cp_result;
        
        debugVerbose(PATH, "Absolute Path %s constructed.\n", (*cpp_path));
        return;
    }
    debugVerbose(PATH, "Path %s is already an absolute path.\n", (*cpp_path));
}

bool mapRequestPath(char** cpp_final_path, bool *cb_static)
{
    char* cp_cgi_bin = "/cgi-bin";
    char* cp_web_dir = "/";
    char* cp_relative_path_without_first_letter = NULL;
    char* cp_relative_path_without_cgi_bin = NULL;
    char* cp_relative_path = NULL;
    int i_cgi_bin_len = strlen(cp_cgi_bin);
    int i_web_dir_len = strlen(cp_web_dir);
    
    if (!http_request_->cp_path)
        return FALSE;
    
    strAppend(&cp_relative_path, http_request_->cp_path);
    int i_relative_path_len = strlen(cp_relative_path);
    
    deleteCyclesFromPath(&cp_relative_path);

    strAppend(&cp_relative_path_without_first_letter, cp_relative_path);
    
    if (i_relative_path_len >= i_cgi_bin_len && strncmp(cp_relative_path, cp_cgi_bin, i_cgi_bin_len) == 0)
    {
        strAppend(cpp_final_path, scp_cgi_dir_);
        strAppend(&cp_relative_path_without_cgi_bin, cp_relative_path_without_first_letter + i_cgi_bin_len);
        strAppend(cpp_final_path, cp_relative_path_without_cgi_bin);
        
        if (convertToRealPath(cpp_final_path) == FALSE)
        {
            secFree(cp_relative_path_without_first_letter);
            secFree(cp_relative_path_without_cgi_bin);
            secFree(cp_relative_path);
            secExit(STATUS_NOT_FOUND);
            return FALSE;
        }
        
        (*cb_static) = FALSE;
        debugVerbose(PATH, "Mapping the request Path: Request is DYNAMIC\n");

        secFree(cp_relative_path_without_cgi_bin);
    }
    else
    {
        if (i_relative_path_len >= i_web_dir_len && strncmp(cp_relative_path, cp_web_dir, i_web_dir_len) == 0)
        {
            strAppend(cpp_final_path, scp_web_dir_);
            strAppend(cpp_final_path, cp_relative_path_without_first_letter);
            
            if (convertToRealPath(cpp_final_path) == FALSE)
            {
                secFree(cp_relative_path_without_first_letter);
                secFree(cp_relative_path);
                secExit(STATUS_NOT_FOUND);
                return FALSE;
            }
            
            (*cb_static) = TRUE;
            
            /** Does the request path now map to the cgi-dir?
             *
             *  This could happen if eg:
             *
             *      cgi-dir:  home/foo/webroot
             *      web-dir:  home/foo/webroot/cgi
             *      Request:  /cgi/script
             *
             *      The Mapped Request would now be 'home/foo/webroot/cgi/script' and Dynamic
             *      And that is why the folloging lines of code are necessary!
             */
            if (checkIfFirstDirContainsSecondDir(scp_cgi_dir_, *cpp_final_path) == TRUE)
            {
                (*cb_static) = FALSE;
            }
            
            if (*cb_static)
                debugVerbose(PATH, "Mapping the request Path: Request is STATIC\n");
            else
                debugVerbose(PATH, "Mapping the request Path: Request is DYNAMIC\n");
        }
        else
        {
            debugVerbose(PATH, "ERROR, mapping request-path to filesystem path did not work!\n");
            secFree(cp_relative_path_without_first_letter);
            secFree(cp_relative_path);
            secExit(STATUS_BAD_REQUEST);
            return FALSE;
        }
    }
    
    secFree(cp_relative_path_without_first_letter);
    secFree(cp_relative_path);
    
    /** Check if we moved out of cgi-dir or web-dir
     *
     *  Sample:     web-dir:        home/foo/web
     *              Request:        /../../some-important-system-file
     *              Mapped Request: home/some-important-system-file
     */
    if (!( (checkIfFirstDirContainsSecondDir(scp_cgi_dir_, *cpp_final_path) == TRUE) ||
           (checkIfFirstDirContainsSecondDir(scp_web_dir_, *cpp_final_path) == TRUE) ))
    {
        debug(PATH, "ERROR, request Path would leave web/cgi directory!\n");
        secExit(STATUS_FORBIDDEN);
    }
    
    return TRUE;
}
