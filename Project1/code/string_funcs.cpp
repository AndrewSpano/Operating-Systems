#include "string_funcs.h"
#include <iostream>


int strlen(const char* str)
{
  int i = 0;
  while (str[i++]);
  return i - 1;
}


int strcmp(const char *str1, const char *str2)
{
  int i = 0;
  for ( ; str1[i] == str2[i] ; i++)
    if (!str1[i])
      return 0;
  return str1[i] - str2[i];
}


void strcpy(char* str1, const char* str2)
{
  int i = 0;
  while (str1[i] = str2[i]) {
    i++;
  }
}


void get_first_string(char* first_string, const char buf[])
{
  int i = 0;
  while ( i < 256 && buf[i] != '\0' && buf[i] != ' ' && buf[i] != '\n') {
    first_string[i] = buf[i];
    i++;
  }
  first_string[i] = '\0';
}


/* function that takes a string (2nd parameter) with multiple sub-strings in it, and copies the n-th string (n is the 3rd parameter)
   in anoter string (1st parameter). Returns true if it manages to make the copy, else returns false if it fails for some reason. */
bool get_nth_string(char* str, const char buf[], int n)
{
  int i = 0;
  for (int j = 1; j <= n - 1; j++) {

    while (i < 256 && buf[i] != ' ' && buf[i] != '\t' && buf[i] != '\n' && buf[i] != '\0') {
      i++;
    }

    if (i >= 256 || (buf[i] != ' ' && buf[i] != '\t')) {
      return false;
    }

    while (i < 256 && (buf[i] == ' ' || buf[i] == '\t')) {
      i++;
    }

  }

  if (i >= 256 || (buf[i] == '\n' || buf[i] == '\0')) {
    return false;
  }

  int k = 0;
  while (i < 256 && buf[i] != ' ' && buf[i] != '\t' && buf[i] != '\n' && buf[i] != '\0') {
    str[k] = buf[i];
    i++;
    k++;
  }
  str[k] = '\0';

  return true;

}


/* function used to read the record fields from a buffer. Returns false is it fails to copy all the sub-strings of the buffer
  to the correspoing strings. Else, if the copy is successful, returns true. */
bool get_record(char* key,  char* firstname, char* lastname, char* year, char* sex, char* postcode, const char buf[])
{
  if (get_nth_string(key, buf, 2) == false) {
    return false;
  }
  if (get_nth_string(firstname, buf, 3) == false) {
    return false;
  }
  if (get_nth_string(lastname, buf, 4) == false) {
    return false;
  }
  if (get_nth_string(year, buf, 5) == false) {
    return false;
  }
  if (get_nth_string(sex, buf, 6) == false) {
    return false;
  }
  if (get_nth_string(postcode, buf, 7) == false) {
    return false;
  }

  return true;
}
