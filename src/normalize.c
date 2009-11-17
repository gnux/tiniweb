#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "typedef.h"
#include "secmem.h"
#include "normalize.h"

static const char *SCCP_BLANK = {" \t"};
static const char *SCCPA_NEW_LINE[] = {"\r\n", "\n", "\0"};


http_norm *normaliseHttp(const FILE* cfp_input){
  http_norm *hnp_http_info;
  hnp_http_info = secMalloc(sizeof(hnp_http_info));
  char *cp_current_line;
  //first char must be valid and no Space
 // cp_current_line = getline()
  // TODO: sec_abort!
  if(isCharacter(cfp_input, 0) != EXIT_SUCCESS)
    return EXIT_FAILURE;
 // cp_current_line = 
  return EXIT_FAILURE;
}

int isBlank(const char* cpp_input, const int i_offset){
  for(int i = 0; i < strlen(SCCP_BLANK); ++i)
    if(cpp_input[i_offset] == SCCP_BLANK[i])
      return EXIT_SUCCESS;
  return EXIT_FAILURE;
}

int isNewLineChars(const char* cpp_input, const int i_offset){
  int i = 0;
  char *cp_current_delim;
  while(1){
    cp_current_delim = SCCPA_NEW_LINE[i];
    if(cp_current_delim[0] == '\0')
      break;
    for(int j=0; j < strlen(cp_current_delim); ++j){
      if(cp_current_delim[j] == '\0')
	return EXIT_SUCCESS;
      else if(cp_current_delim[j] != cpp_input[i_offset + j])
	break;
    }
  }
  return EXIT_FAILURE;
}

int isCharacter(const char* cpp_input, const int i_offset){
  return (cpp_input[i_offset] >= 0x21 && cpp_input[i_offset] <= 0x7e) ? EXIT_FAILURE : EXIT_SUCCESS;
}