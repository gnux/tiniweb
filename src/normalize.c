#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "typedef.h"
#include "secmem.h"
#include "normalize.h"
#include "debug.h"

static const char *SCCP_BLANK = {" \t"};
static const char *SCCPA_NEW_LINE[] = {"\r\n", "\n", "\0"};

//TODO: proof every line first for valid chars!!! sec getline???

http_norm *normalizeHttp(FILE* fp_input){
  http_norm *hnp_http_info;
  hnp_http_info = secMalloc(sizeof(http_norm));
  hnp_http_info->i_num_fields = 0;
  hnp_http_info->cpp_header_field_name = NULL;
  hnp_http_info->cpp_header_field_body = NULL;
  hnp_http_info->cp_header = NULL;
  hnp_http_info->cp_body = NULL;
  char *cp_current_line = NULL;
  size_t i_num_read = 0;
  // first char must be valid and no space
  // TODO: sec_abort!
  // TODO: What is when malloc in getline fails??? secGetline??
  if(getline(&cp_current_line, &i_num_read, fp_input) == -1)
    abort();
  // TODO: sec_abort!
  if(isCharacter(cp_current_line, 0) == EXIT_FAILURE)
    abort();
  hnp_http_info->cp_first_line = secCalloc(i_num_read + 1, sizeof(char));
  strncpy(hnp_http_info->cp_first_line, cp_current_line, i_num_read + 1);
  // now find the first line (http request line)
  while(1){
    if(cp_current_line)
      free(cp_current_line);
    cp_current_line = NULL;
    // because there must be add least on header field... we will abort if we found non
    // TODO: sec_abort!
    i_num_read = 0;
    if(getline(&cp_current_line, &i_num_read, fp_input) == -1)
      abort();
    // if we find a newline on the beginning of the line, we are still missing at least one header-field
    // TODO: sec_abort!
    if(isNewLineChars(cp_current_line, 0) == EXIT_SUCCESS)
      abort();
    // we do this as long we find a char on first position
    if(isCharacter(cp_current_line, 0) == EXIT_SUCCESS)
      break;
    // just append current line to the first line
    strAppend(&hnp_http_info->cp_first_line, cp_current_line);
  }
  // now hunt for header-fields
  while(1){
    //TODO: sec abort
    if(isValidHeaderFieldStart(cp_current_line) == EXIT_FAILURE)
      abort();
    ++hnp_http_info->i_num_fields;
    hnp_http_info->cpp_header_field_name = secRealloc(hnp_http_info->cpp_header_field_name, sizeof(char*) * hnp_http_info->i_num_fields);
    getHeaderFieldName(&hnp_http_info->cpp_header_field_name[hnp_http_info->i_num_fields - 1], cp_current_line);
    hnp_http_info->cpp_header_field_body = secRealloc(hnp_http_info->cpp_header_field_body, sizeof(char*) * hnp_http_info->i_num_fields);
    getHeaderFieldBody(&hnp_http_info->cpp_header_field_body[hnp_http_info->i_num_fields - 1], cp_current_line);
    // eat away multirow things
    do{
      if(cp_current_line)
	free(cp_current_line);
      // TODO: sec_abort! sec getline with auto abort!
      // Header has to end with endl!
      i_num_read = 0;
      if(getline(&cp_current_line, &i_num_read, fp_input) == -1)
	abort();
      if(isBlank(cp_current_line, 0) == EXIT_SUCCESS)
	strAppend(&hnp_http_info->cpp_header_field_body[hnp_http_info->i_num_fields - 1], cp_current_line);
      else
	break;
    }while(1);
    // finished
    if(isNewLineChars(cp_current_line,0) == EXIT_SUCCESS)
      break;
  }
  free(cp_current_line);
  normalizeHeaderFields(hnp_http_info);
  restoreNormalizedHeader(hnp_http_info);
  hnp_http_info->cp_body = secCalloc(1,sizeof(char));
  hnp_http_info->cp_body[0] = '\0';
  i_num_read = 0;
  while(getline(&cp_current_line, &i_num_read, fp_input) != -1){
    strAppend(&hnp_http_info->cp_body, cp_current_line);
    if(cp_current_line)
      free(cp_current_line);
    i_num_read = 0;
  }
  if(cp_current_line)
      free(cp_current_line);
  printHttpNorm(hnp_http_info);
  return hnp_http_info;
}

void restoreNormalizedHeader(http_norm* hnp_http_info){
  size_t i;
  hnp_http_info->cp_header = secCalloc(1, sizeof(char));
  hnp_http_info->cp_header[0] = '\0';
  strAppend(&hnp_http_info->cp_header, hnp_http_info->cp_first_line);
  strAppend(&hnp_http_info->cp_header, "\n");
  for(i=0; i<hnp_http_info->i_num_fields; ++i){
    strAppend(&hnp_http_info->cp_header, hnp_http_info->cpp_header_field_name[i]);
    strAppend(&hnp_http_info->cp_header, ": ");
    strAppend(&hnp_http_info->cp_header, hnp_http_info->cpp_header_field_body[i]);
    strAppend(&hnp_http_info->cp_header, "\n");
  }
  strAppend(&hnp_http_info->cp_header, "\n");
}

void normalizeHeaderFields(http_norm* hnp_http_info){
  normalizeSingleLine(&(hnp_http_info->cp_first_line));
  for(size_t i = 0; i < hnp_http_info->i_num_fields; ++i){
    normalizeSingleLine(&hnp_http_info->cpp_header_field_name[i]);
    if(&hnp_http_info->cpp_header_field_body[i])
    normalizeSingleLine(&hnp_http_info->cpp_header_field_body[i]);
  }
}

void normalizeSingleLine(char** cpp_input){
  size_t i_offset = 0;
  size_t i = 0;
  bool b_flag = 0;
  while(isBlankNewLineChars(*cpp_input,i) == EXIT_SUCCESS){
    ++i;}
  i_offset = i;
  for(i=i;(*cpp_input)[i] != '\0'; ++i){
    if(isBlankNewLineChars(*cpp_input, i) == EXIT_SUCCESS){
      b_flag = 1;
      ++i_offset;
    }
    else{
      if(b_flag == 1){
	(*cpp_input)[i-i_offset] = ' ';
	--i_offset;
	b_flag = 0;
      }
      (*cpp_input)[i-i_offset] = (*cpp_input)[i];
    }
  }
  (*cpp_input)[i-i_offset] = '\0';
  *cpp_input = secRealloc(*cpp_input, strlen(*cpp_input) * sizeof(char) + 1);
}

void printHttpNorm(http_norm* hnp_http_info){
  debugVerbose(2, "-----START HEADER STRUCT PRINTING-----\n");
  if(hnp_http_info->cp_first_line)
    debugVerbose(2, "first-line: %s\n", hnp_http_info->cp_first_line);
  for(size_t i = 0; i < hnp_http_info->i_num_fields; ++i)
    debugVerbose(2, "%s: %s\n", hnp_http_info->cpp_header_field_name[i], hnp_http_info->cpp_header_field_body[i]);
  if(hnp_http_info->cp_header)
    debugVerbose(2, "complete request/response:\n%s%s", hnp_http_info->cp_header, hnp_http_info->cp_body);
//   if(hnp_http_info->cp_body)
//     debugVerbose(2, "complete_body:\n%s\n", hnp_http_info->cp_body);
  debugVerbose(2, "-----END HEADER STRUCT PRINTING-----\n");
}

void getHeaderFieldName(char** cpp_output, const char* ccp_input){
  size_t i_last_char_name;
  // There must be an blank or an ':'
  for(i_last_char_name = 0; isBlank(ccp_input, i_last_char_name) == EXIT_FAILURE && ccp_input[i_last_char_name] != ':'; ++i_last_char_name)
  *cpp_output = secCalloc(i_last_char_name + 2, sizeof(char));
  strncpy(*cpp_output, ccp_input, i_last_char_name);
  (*cpp_output)[i_last_char_name] = '\0';  
}

void getHeaderFieldBody(char** cpp_output, const char* ccp_input){
  size_t i_offset_token;
  size_t i_len_output;
  // search for ':' token, there must be one, we've already checked that
  for(i_offset_token = 0; ccp_input[i_offset_token] != ':'; ++i_offset_token);
  i_len_output = strlen(ccp_input) - i_offset_token;
  // TODO: discuss of defining MAX_STR_LEN ~ 100MB --- strnlen function!
  *cpp_output = secCalloc(i_len_output, sizeof(char));
  strncpy(*cpp_output, &ccp_input[i_offset_token + 1], i_len_output);
  (*cpp_output)[i_len_output - 1] = '\0';
}

int isBlank(const char* ccp_input, const size_t i_offset){
  for(size_t i = 0; SCCP_BLANK[i] != '\0'; ++i)
    if(ccp_input[i_offset] == SCCP_BLANK[i])
      return EXIT_SUCCESS;
  return EXIT_FAILURE;
}

int isNewLineChars(const char* ccp_input, const size_t i_offset){
  size_t i = 0;
  size_t j = 0;
  while(1){
    if(SCCPA_NEW_LINE[i][0] == '\0')
      return EXIT_FAILURE;
    for(j=0; SCCPA_NEW_LINE[i][j]; ++j)
      if(SCCPA_NEW_LINE[i][j] != ccp_input[i_offset + j])
        break;
    if(SCCPA_NEW_LINE[i][j] == '\0')
      return EXIT_SUCCESS;
    ++i;
  }
  return EXIT_FAILURE;
}

int isBlankNewLineChars(const char* ccp_input, const size_t i_offset){
  return ((isBlank(ccp_input, i_offset) == EXIT_SUCCESS) ? EXIT_SUCCESS : isNewLineChars(ccp_input, i_offset));
}

int isCharacter(const char* cpp_input, const size_t i_offset){
  return (cpp_input[i_offset] > 32 && cpp_input[i_offset] < 128) ? EXIT_SUCCESS : EXIT_FAILURE;
}

void strAppend(char** cpp_output, const char* ccp_input){
  size_t i_len_input = strlen(ccp_input);
  size_t i_len_output = strlen(*cpp_output);
  size_t i_len_new = i_len_input + i_len_output + 1;
  // prevent overflow
  // TODO: sec abort!
  // TODO: search for possible overflows!
  if(i_len_new < i_len_input || i_len_new < i_len_output)
    abort();
  *cpp_output = secRealloc(*cpp_output, i_len_new);
  strncat(*cpp_output, ccp_input, i_len_input);
  // just be sure to delimit with '\0'
  (*cpp_output)[i_len_new - 1] = '\0';
}

int isValidHeaderFieldStart(const char* ccp_input){
  size_t i_offset_token;
  size_t i_last_char_name;
  size_t i;
  // search for ':' token
  for(i_offset_token = 0; i_offset_token < strlen(ccp_input); ++i_offset_token)
    if(ccp_input[i_offset_token] == ':')
      break;
  // no name given
    if(i_offset_token == strlen(ccp_input) || i_offset_token == 0)
    return EXIT_FAILURE;
  // search for last char in name
  for(i_last_char_name = 0; i_last_char_name < i_offset_token; ++i_last_char_name)
    if(isBlank(ccp_input, i_last_char_name) == EXIT_SUCCESS)
      break; 
  // now proof if entries from i_last_char_name to i_offset_token is filled only by spaces
  for(i = i_last_char_name; i < i_offset_token; ++i)
    if(isBlank(ccp_input, i) == EXIT_SUCCESS)
      return EXIT_FAILURE;
  return EXIT_SUCCESS;
}