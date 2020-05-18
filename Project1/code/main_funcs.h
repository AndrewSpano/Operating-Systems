#ifndef __MAIN_FUNCS__
#define __MAIN_FUNCS__

#include "red_black_tree.h"
#include "catalogue.h"
#include "bloom_filter.h"
#include "string_funcs.h"

bool get_paremeters(char** inputfile, char** outfile, unsigned int* numofupdates, int argc, char* argv[]);
void get_voters(BFPtr* bf, RBTPtr rbT, CataloguePtr catalogue, char* inputfile, unsigned int* num_of_records, const unsigned int numofupdates);

unsigned long get_BF_size(const unsigned int num_of_records);
unsigned long find_prime_bigger_than(unsigned long n);
bool is_prime(const unsigned long n);

unsigned short get_option(char buf[]);


#endif
