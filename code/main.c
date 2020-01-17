#include <stdio.h>
#include <stdlib.h>

#include "functions.h"
#include "util.h"
#include "list.h"



int main(int argc, char* argv[])
{

  int fd = cfs_create("test.cfs", 512, 30, 5000, 10);
  cfs_read("test.cfs", fd);

  printf("\n\n");
  return EXIT_SUCCESS;
}
