
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "path.h"
#include "debug.h"
#include "normalize.h"
#include "secmem.h"

bool performPathChecking(char** cpp_path_cgi, char** cpp_path_web)
{
    constructAbsolutePath(cpp_path_cgi);
    constructAbsolutePath(cpp_path_web);
    deleteCyclesFromPath(cpp_path_cgi);
    deleteCyclesFromPath(cpp_path_web);
    
    if (checkPath(*cpp_path_cgi) == FALSE || checkPath(*cpp_path_web) == FALSE)
    {
        return FALSE;
    }
    
    if (checkIfCGIDirContainsWebDir(*cpp_path_cgi, *cpp_path_web) == TRUE)
    {
        return FALSE;
    }
    
    debug(PATH, "Success: Path checking finished! No error ocurred!\n");
    return TRUE;
}

bool checkPath(char* ca_path)
{
    struct stat buffer;
    int i_result = stat(ca_path, &buffer);
    
    if (i_result == 0)
    {
        debugVerbose(PATH, "Success: Directory Checked: Directory/File %s is valid!\n", ca_path);
        return TRUE;
    }

    debugVerbose(PATH, "ERROR: Directory Checked: Directory/File %s is NOT valid!\n", ca_path);
    return FALSE;
}

bool checkIfCGIDirContainsWebDir(char* ca_path_cgi, char* ca_path_web)
{
    int i_path_cgi_len = strlen(ca_path_cgi);
    int i_path_web_len = strlen(ca_path_web);
    int i_shortest_path_len = 0;
    int i = 0;
    bool b_paths_do_not_contain_each_other = FALSE;
    
    if (i_path_web_len >= i_path_cgi_len)
    {
        i_shortest_path_len = (i_path_cgi_len > i_path_web_len) ?  i_path_web_len : i_path_cgi_len;
    
        for (i = 0; i < i_shortest_path_len; i++)
        {
            int i_result = strncmp(ca_path_cgi, ca_path_web, i);
            if (i_result != 0)
            {
                b_paths_do_not_contain_each_other = TRUE;
                break;
            }
        }
    }
    
    if (!b_paths_do_not_contain_each_other)
        debugVerbose(PATH, "Success: CGI-Dir: '%s' does not contain WEB-Dir: '%s'!\n", ca_path_cgi, ca_path_web);
    else
        debugVerbose(PATH, "ERROR: CGI-Dir: '%s' contains WEB-Dir: '%s'!\n", ca_path_cgi, ca_path_web);
    
    return b_paths_do_not_contain_each_other;
}

void deleteCyclesFromPath(char** cpp_path_to_check)
{
    int i_path_len = strlen(*cpp_path_to_check);
    char** cpp_path = NULL;
    char* cp_result_path = NULL;
    int i_alloc_result_path_len = 0;
    int i_num_folders = 0;
    int i_start = 0;
    
    /**
     *  Store the path into the sorted cpp_path. The result will be e.g:
     *
     *   cpp_path[0] = "Juhu"
     *   cpp_path[0][0] = 'J'
     */
    for (int i_end = 0; i_end < i_path_len; i_end++)
    {
//         debugVerbose(PATH, "i_end: %i\n", i_end);
        
        if ((*cpp_path_to_check)[i_end] == '/' || i_end == i_path_len - 1)
        {
//             debugVerbose(PATH, "'/' Found!\n");

            int i_num_chars = i_end - i_start + 1;
//             debugVerbose(PATH, "  i_num_chars = %i\n", i_num_chars);

            if (i_num_folders == 0)
                cpp_path = (char**) secMalloc((i_num_folders + 1) * sizeof(char*));
            else
                cpp_path = (char**) secRealloc(cpp_path, (i_num_folders + 1) * sizeof(char*));
            
            cpp_path[i_num_folders] = (char*) secMalloc((i_num_chars + 1) * sizeof(char));
            
//             debugVerbose(PATH, "nach malloc!\n");
            
            // Write all chars into new sorted array
            strncpy(cpp_path[i_num_folders], (*cpp_path_to_check) + i_start, i_num_chars);
            cpp_path[i_num_folders][i_num_chars] = '\0';
//             debugVerbose(PATH, "Write from A to B: %s\n", cpp_path[i_num_folders]);
            
            i_start = i_end + 1;
            i_num_folders++;
        }
    }
    
    /**
     *  Perform the checking if '../' or './' are in the path
     *   
     *   
     */
    for (int i_current_folder = 0; i_current_folder < i_num_folders; i_current_folder++)
    {
//         for (int i = 0; i < i_num_folders; i++) {
//             debugVerbose(PATH, "BEFORE: i_current_folder: %i, String: %s\n", i, cpp_path[i]);
//         }
        
        if (strncmp(cpp_path[i_current_folder], "../", 3) == 0 ||
            strncmp(cpp_path[i_current_folder], "..", 2) == 0)
        {
            secRealloc(cpp_path[i_current_folder], 1 * sizeof(char));
            cpp_path[i_current_folder][0] = '\0';
            if (i_current_folder > 0)
            {

                // Check recursive for the last folder
                for (int i_folder_reverse = i_current_folder - 1; i_folder_reverse >= 0; i_folder_reverse--)
                {
                    if (cpp_path[i_folder_reverse][0] != '\0')
                    {
                        secRealloc(cpp_path[i_folder_reverse], 1 * sizeof(char));
                        cpp_path[i_folder_reverse][0] = '\0';
                        break;
                    }
                }
            }
        }
        else if ( strncmp(cpp_path[i_current_folder], "./", 2) == 0 || 
                ( cpp_path[i_current_folder][0] == '.' && cpp_path[i_current_folder][1] == '\0' ) )
        {
            secRealloc(cpp_path[i_current_folder], 1 * sizeof(char));
            cpp_path[i_current_folder][0] = '\0';
        }
        
//         for (int i = 0; i < i_num_folders; i++) {
//             debugVerbose(PATH, "AFTER:  i_current_folder: %i, String: %s\n", i, cpp_path[i]);
//         }
    }
    
    /**
     *  Store the result into the resultstring.
     */
    for (int i_current_folder = 0; i_current_folder < i_num_folders; i_current_folder++)
    {
        if (cpp_path[i_current_folder][0] != '\0')
        {
            int i_str_len = strlen(cpp_path[i_current_folder]);
            cp_result_path = secRealloc(cp_result_path, i_alloc_result_path_len + i_str_len);
            
            strncpy(cp_result_path + (i_alloc_result_path_len), cpp_path[i_current_folder], i_str_len);
            i_alloc_result_path_len += i_str_len;
        }
        
//         debugVerbose(PATH, "Currentfolder: %i, Resultstring: %s\n", i_current_folder, cp_result_path);
    }
    cp_result_path[i_alloc_result_path_len] = '\0';
    
    /**
     *  Free Allocated Memory
     */
    for (int i_current_folder = 0; i_current_folder < i_num_folders; i_current_folder++)
        secFree(&(cpp_path[i_current_folder]));
    secFree(cpp_path);
    
    debugVerbose(PATH, "Removed All cycles from %s\n", (*cpp_path_to_check));
    (*cpp_path_to_check) = cp_result_path;
//     debugVerbose(PATH, "    Result: %s\n", (*cpp_path_to_check));
}

void getSortedPath(char* cp_path_to_sort, char** cpp_path)
{
    
    
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
//         int i_path_len = strlen(*cpp_path);
        cp_buffer = secMalloc(i_buffer_len * sizeof(char));
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
        (*cpp_path) = cp_result;
        
        debugVerbose(PATH, "Absolute Path %s constructed.\n", (*cpp_path));
        return;
    }
    debugVerbose(PATH, "Path %s is already an absolute path.\n", (*cpp_path));
}

void testPathChecking()
{
        checkPath("../cgi-bin");
    checkPath("../cgi-bin/foo");
    checkPath("..");
    checkPath("");
    checkPath("../src/../src/auth.c");
    checkPath("/etc/");
    checkPath("/etc/foooooooo");
    checkPath("/..");
    
    fprintf(stderr, "###########################################\n");
    
    char* cp_path_1 = "/foo/../foo";
    char* cp_path_2 = "/../foo/../foo/././juhu";
    performPathChecking(&cp_path_1,&cp_path_2);
    
    char* cp_path_3 = "../../"; // TODO BUG!!!
    char* cp_path_4 = "/.";
    performPathChecking(&cp_path_3,&cp_path_4);
}

