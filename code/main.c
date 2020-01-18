#include <stdio.h>
#include <stdlib.h>

#include "functions.h"
#include "functions2.h"
#include "util.h"
#include "list.h"



int main(int argc, char* argv[])
{

  int fd = cfs_create("test.cfs", 512, 30, 5000, 10);
  cfs_read("test.cfs", fd);

  printf("\n\n");


  size_t off; 
  off = find_hole("test.cfs", fd, sizeof(MDS));
  printf("----------->main:%lu\n", off);
  return EXIT_SUCCESS;
}
