#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include "main_funcs.h"

using namespace std;


bool get_paremeters(char** inputfile, char** outfile, unsigned int* numofupdates, int argc, char* argv[])
{
  bool flag_input = false;
  for (int i = 1; i <= argc - 2; i += 2 ) {

    if (strcmp(argv[i], "-i") == 0) {

      *inputfile = new char[strlen(argv[i + 1]) + 1];
      strcpy(*inputfile, argv[i + 1]);
      flag_input = true;

    } else if (strcmp(argv[i], "-o") == 0) {

      *outfile = new char[strlen(argv[i + 1]) + 1];
      strcpy(*outfile, argv[i + 1]);

    } else if (strcmp(argv[i], "-n") == 0) {

      *numofupdates = atoi(argv[i + 1]);

    } else {

      cout << "\n\nWrong command line arguments given.\n\n";

    }

  }

  return flag_input;
}



void get_voters(BFPtr* bf, RBTPtr rbT, CataloguePtr catalogue, char* inputfile, unsigned int* num_of_records, const unsigned int numofupdates)
{

  FILE* registry = fopen(inputfile, "r");
  if (registry == NULL) {
    cout << "There was a problem opening the file.\n";
    exit(1);
  }

  char ch = fgetc(registry);
  if (feof(registry)) {
    cout << "Error, the recodrs file is empty.\n";
    exit(2);
  }

  while (!feof(registry)) {
    fseek(registry, -1, SEEK_CUR);
    char* id = new char[20];
    char* name = new char[20];
    char* surname = new char[20];
    int age = 0;
    char sex = 'F';
    int postcode = 0;

    fscanf(registry, "%s %s %s %d %c %d", id, name, surname, &age, &sex, &postcode);

    if (rbT->search(id) == false) {
      voter* v = new voter(id, name, surname, age, (sex == 'M'), postcode);
      rbT->insert(v);
      catalogue->add_person_to_postal_code(postcode);
      (*num_of_records)++;
    } else {
      delete[] id;
      delete[] name;
      delete[] surname;
    }


    ch = fgetc(registry);
    ch = fgetc(registry);
  }


  unsigned int bf_size = get_BF_size(rbT->size);
  *bf = new BF(bf_size, numofupdates, rbT);

  fclose(registry);

}




unsigned long get_BF_size(const unsigned int num_of_records)
{
  return find_prime_bigger_than(3 * num_of_records);
}



unsigned long find_prime_bigger_than(unsigned long n)
{
  while (!is_prime(++n));
  return n;
}



bool is_prime(const unsigned long n)
{
  if (n % 2 == 0) {
    return false;
  }

  for (int i = 3; i*i <= n; i += 2) {
    if (n % i == 0) {
      return false;
    }
  }

  return true;
}



unsigned short get_option(char buf[])
{
  unsigned short option = 0;
  char* first_string = new char[256];

  get_first_string(first_string, buf);

  if (strcmp(first_string, "lbf") == 0) {
    option = 1;
  } else if (strcmp(first_string, "lrb") == 0) {
    option = 2;
  } else if (strcmp(first_string, "ins") == 0) {
    option = 3;
  } else if (strcmp(first_string, "find") == 0) {
    option = 4;
  } else if (strcmp(first_string, "delete") == 0) {
    option = 5;
  } else if (strcmp(first_string, "vote") == 0) {
    option = 6;
  } else if (strcmp(first_string, "load") == 0) {
    option = 7;
  } else if (strcmp(first_string, "voted") == 0 && strcmp(buf, "voted\n") == 0) {
    option = 8;
  } else if (strcmp(first_string, "voted") == 0) {
    option = 9;
  } else if (strcmp(first_string, "votedperpc") == 0) {
    option = 10;
  } else if (strcmp(first_string, "exit") == 0) {
    option = 11;
  } else {
    option = 0;
  }


  delete[] first_string;
  return option;
}
