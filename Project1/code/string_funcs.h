#ifndef __STR_FUNCS__
#define __STR_FUNCS__

int strlen(const char* str);
int strcmp(const char* str1, const char* str2);
void strcpy(char* str1, const char* str2);
void get_first_string(char* first_string, const char buf[]);
bool get_nth_string(char* str, const char buf[], int n);
bool get_record(char* key,  char* firstname, char* lastname, char* year, char* sex, char* postcode, const char buf[]);

#endif
