#ifndef __UTIL__
#define __UTIL__

#include "structs.h"

#define MAX_BUFFER_SIZE 512

int get_nth_string(char* str, const char buf[], int n);
int get_option(const char buffer[]);

void initialize_holes(hole_map* holes, int n);
void print_superblock(superblock* my_superblock);
void print_hole_table(hole_map* holes);
void print_MDS(MDS* mds);

#endif
