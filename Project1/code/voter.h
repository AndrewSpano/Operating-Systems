#ifndef __VOTER__
#define __VOTER__

#include "string_funcs.h"

#define FEMALE false
#define MALE true

struct voter {
  char* id;
  char* name;
  char* surname;
  const short age;
  const bool sex;
  const int postal_code;

  voter(char* Id, char* Name, char* Surname, short Age, bool Sex, int Postal_code);
  ~voter(void);

  short compare(voter* v);
  short compare(char* id);
  void print(FILE* output);
};




#endif
