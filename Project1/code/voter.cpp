#include <iostream>
#include <stdio.h>
#include "voter.h"

using namespace std;

voter::voter(char* Id, char* Name, char* Surname, short Age, bool Sex, int Postal_code): id(Id), name(Name), surname(Surname), age(Age), sex(Sex), postal_code(Postal_code)
{ /* cout << "A new voter has been created!\n"; */ }


voter::~voter(void)
{ delete[] id; delete[] name; delete[] surname; /* cout << "A voter has been destructed!\n"; */ }


/* returns -1 if this->id < v->id,
   returns 1 if this->id > v->id and
   returns 0 if they are equal */
short voter::compare(voter* v)
{
  return strcmp(id, v->id);
}


short voter::compare(char* str)
{
  return -strcmp(id, str);
}


void voter::print(FILE* output)
{
  // cout << id << "  " << name << "  " << surname << "  " << age << "  " << (sex ? 'M' : 'F') << "  " << postal_code << endl;
  fprintf(output, "%s  %s  %s %d %c %d\n", id, name, surname, age, (sex ? 'M' : 'F'), postal_code);
}
