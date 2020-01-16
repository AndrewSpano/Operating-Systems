#include <stdio.h>
#include <stdlib.h>
#include "functions.h"

#include "util.h"



int main(int argc, char* argv[])
{

  int fd = cfs_create("test.cfs", 512, 30, 5000, 10);
  cfs_read(fd);

  // printf("time %lu, size %lu, uint %lu\n", sizeof(time_t), sizeof(size_t), sizeof(uint));

  return EXIT_SUCCESS;
}
