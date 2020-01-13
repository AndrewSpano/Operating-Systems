#include <stdio.h>
#include <stdlib.h>

#include "util.h"



int main(int argc, char* argv[])
{

  char my_buf[256];
  char* buffer = "cfs_export";

  printf("func: %d\n", get_option(buffer));

  return EXIT_SUCCESS;
}
